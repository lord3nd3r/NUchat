#include "IRCConnectionManager.h"
#include "CredentialRedactor.h"
#include "NetSafety.h"
#include "DccManager.h"
#include "IrcConnection.h"
#include "IrcIdentity.h"
#include "Logger.h"
#include "MessageModel.h"
#include "ServerChannelModel.h"
#include "PluginManager.h"
#include "ScriptDispatch.h"
#include "ScriptEat.h"
#include "Settings.h"
#include "UserPaths.h"
#include "Version.h"
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QNetworkProxy>
#include <QProcess>
#include <QRegularExpression>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <type_traits>

IRCConnectionManager::IRCConnectionManager(QObject *parent) : QObject(parent) {
  // ── Lag meter ── ping every connected server every 30s
  m_lagTimer.setInterval(30000);
  connect(&m_lagTimer, &QTimer::timeout, this, [this]() {
    for (auto *conn : m_connections) {
      if (!conn || !conn->isConnected()) continue;
      auto &state = m_lagState[conn];
      state.pingSent.start();
      state.pending = true;
      // Bypass the flood queue so the measurement excludes queue latency
      conn->sendPing("LAG" + QString::number(state.pingSent.msecsSinceReference()));
    }
  });
}

IRCConnectionManager::~IRCConnectionManager() {
  for (auto *c : m_connections)
    c->disconnectFromServer("NUchat shutting down");
  qDeleteAll(m_connections);
  m_lagState.clear();
}

void IRCConnectionManager::setMessageModel(MessageModel *model) {
  if (m_msgModel)
    disconnect(m_msgModel, nullptr, this, nullptr);
  m_msgModel = model;
  if (!m_msgModel)
    return;
  connect(m_msgModel, &MessageModel::embedReady, this,
          [this](const QString &server, const QString &channel,
                 const QString &html) { rememberEmbed(server, channel, html); });
}

void IRCConnectionManager::setServerChannelModel(ServerChannelModel *model) {
  m_treeModel = model;
}

void IRCConnectionManager::setLogger(Logger *logger) { m_logger = logger; }

void IRCConnectionManager::setSettings(Settings *settings) {
  m_settings = settings;
  // Load ignore list from settings
  if (m_settings) {
    QVariant stored = m_settings->value("ignore/list");
    if (stored.isValid())
      m_ignoreList = stored.toStringList();
  }
}

void IRCConnectionManager::connectToServer(
    const QString &host, int port, bool ssl, const QString &nick,
    const QString &user, const QString &realname, const QString &password,
    const QString &saslMethod, const QString &saslUser, const QString &saslPass,
    const QString &nickServCmd, const QString &nickServPass,
    const QString &autojoin, const QString &perform,
    const QString &displayName) {
  QString name = displayName.isEmpty() ? host : displayName;
  qDebug() << "[Manager] connectToServer called:" << host << port << ssl << nick
           << "sasl:" << saslMethod << "nickserv:" << (!nickServPass.isEmpty())
           << "displayName:" << displayName;

  // If a connection for this host label already exists (e.g. prior failed attempt
  // from network list, or user re-clicked Connect), evict it so we replace under
  // the existing tree row instead of stacking a duplicate. Also clear any stale
  // reconnect timer/info for this host (will be overwritten below).
  if (auto *old = connectionForServer(name))
    retireConnection(old);
  if (m_reconnectInfo.contains(name) && m_reconnectInfo[name].timer) {
    m_reconnectInfo[name].timer->stop();
    m_reconnectInfo[name].timer->deleteLater();
    m_reconnectInfo[name].timer = nullptr;
  }
  m_reconnectInfo.remove(name);

  auto *conn = new IrcConnection(this);
  conn->setNickname(nick);
  conn->setUser(user, realname);
  if (!password.isEmpty())
    conn->setPassword(password);

  // Per-network SASL
  if (!saslMethod.isEmpty() && saslMethod != "None")
    conn->setSaslAuth(saslMethod, saslUser, saslPass);

  // Per-network NickServ auto-identify
  if (!nickServPass.isEmpty()) {
    conn->setNickServCmd(nickServCmd);
    conn->setNickServPass(nickServPass);
  }

  m_connections.append(conn);
  m_connToName[conn] = name;
  // Identity starts at the rfc1459 defaults until this connection's 005.
  publishSupport(name, IrcSupport());

  // Add to tree model
  if (m_treeModel) {
    m_treeModel->addServer(name);
  }

  wireConnection(conn);
  emit clientAdded(conn);

  // Store connection params for auto-reconnect
  ReconnectInfo ri;
  ri.host = host;
  ri.port = port;
  ri.ssl = ssl;
  ri.nick = nick;
  ri.user = user;
  ri.realname = realname;
  ri.password = password;
  ri.saslMethod = saslMethod;
  ri.saslUser = saslUser;
  ri.saslPass = saslPass;
  ri.nickServCmd = nickServCmd;
  ri.nickServPass = nickServPass;
  ri.autojoin = autojoin;
  ri.perform = perform;
  ri.displayName = displayName;
  ri.attempts = 0;
  ri.timer = nullptr;
  m_reconnectInfo[name] = ri;
  m_userDisconnected.remove(name);

  // Set as active server
  m_activeServer = name;
  m_activeChannel = name; // server tab
  if (m_msgModel)
    m_msgModel->setBufferContext(name, name);

  // Record a system message
  if (m_msgModel) {
    m_msgModel->addMessage("system", "Connecting to " + host + ":" +
                                         QString::number(port) +
                                         (ssl ? " (SSL)" : "") + "...");
  }
  appendToChannel(name, name, "system",
                  "Connecting to " + host + ":" + QString::number(port) +
                      "...");

  const QString proxyErr = applyProxySettings(conn);
  if (!proxyErr.isEmpty()) {
    appendToChannel(name, name, "system", proxyErr);
    if (m_msgModel)
      m_msgModel->addMessage("system", proxyErr);
    qWarning() << "[Manager]" << proxyErr;
    return;
  }
  applyTlsPreferences(conn);
  conn->connectToServer(host, static_cast<quint16>(port), ssl);
}

void IRCConnectionManager::disconnectFromServer(const QString &host) {
  m_userDisconnected.insert(host);
  // Cancel any pending reconnect timer
  if (m_reconnectInfo.contains(host) && m_reconnectInfo[host].timer) {
    m_reconnectInfo[host].timer->stop();
    m_reconnectInfo[host].timer->deleteLater();
    m_reconnectInfo[host].timer = nullptr;
    m_reconnectInfo[host].attempts = 0;
  }
  if (auto *conn = connectionForServer(host)) {
    conn->disconnectFromServer();
  }
}

void IRCConnectionManager::disconnectAll() {
  for (auto *c : m_connections)
    m_userDisconnected.insert(serverNameFor(c));
  for (auto *c : m_connections)
    c->disconnectFromServer();
}

void IRCConnectionManager::joinChannel(const QString &channel,
                                       const QString &key) {
  if (auto *conn = activeConnection()) {
    conn->joinChannel(channel, key);
  }
}

void IRCConnectionManager::partChannel(const QString &channel,
                                       const QString &reason) {
  if (auto *conn = activeConnection()) {
    conn->partChannel(channel, reason);
  }
}

void IRCConnectionManager::closeChannel(const QString &serverName,
                                        const QString &channelName) {
  if (isChannelName(serverName, channelName)) {
    if (auto *conn = connectionForServer(serverName)) {
      conn->partChannel(channelName);
    }
  }
  if (m_treeModel) {
    m_treeModel->removeChannel(serverName, channelName);
  }
  emit channelParted(serverName, channelName);
  cleanupChannelState(serverName, channelName);
}

void IRCConnectionManager::closeServer(const QString &serverName) {
  m_userDisconnected.insert(serverName);

  // Cancel any pending reconnect timer and remove stored info for this server
  if (m_reconnectInfo.contains(serverName) && m_reconnectInfo[serverName].timer) {
    m_reconnectInfo[serverName].timer->stop();
    m_reconnectInfo[serverName].timer->deleteLater();
    m_reconnectInfo[serverName].timer = nullptr;
    m_reconnectInfo[serverName].attempts = 0;
  }
  m_reconnectInfo.remove(serverName);

  // Disconnect and clean up the live connection (if any)
  if (auto *conn = connectionForServer(serverName)) {
    if (conn->isConnected()) {
      conn->disconnectFromServer("Closing server tab");
    }
    retireConnection(conn);
  }

  // Remove the top-level server entry from the tree (this is what "close tab" means for servers)
  if (m_treeModel) {
    m_treeModel->removeServer(serverName);
  }

  // If this was the active server, clear the active context (UI will react via bindings)
  if (ircSame(m_activeServer, serverName)) {
    m_activeServer = QString();
    m_activeChannel = QString();
  }

  // Best-effort cleanup of any per-channel state that might be lingering
  cleanupChannelState(serverName, serverName);

  // Notify QML so the sidebar list model can refresh and selection can be cleaned up
  emit serverClosed(serverName);
}

void IRCConnectionManager::sendMessage(const QString &target,
                                       const QString &message) {
  auto *conn = activeConnection();
  if (!conn)
    return;

  const OutboundLine outbound = classifyOutbound(message);
  if (outbound.kind == OutboundKind::Command) {
    handleSlashCommand(conn, target, outbound.command, outbound.args);
    return;
  }
  const QString literal = outbound.literal;

  // Regular message (including "//text" and path-like "/etc/passwd")
  conn->sendMessage(target, literal);

  // Show our own message locally with status prefix
  QString nick = conn->nickname();
  QString displayNick = nick;
  ChannelKey key{m_activeServer, target};
  if (m_users.contains(key)) {
    for (const QString &u : m_users[key]) {
      auto [pfx, bare] = conn->support().splitNick(u);
      if (conn->support().same(bare, nick)) {
        if (!pfx.isEmpty())
          displayNick = pfx[0] + nick;
        break;
      }
    }
  }
  const QString shown = CredentialRedactor::redactOutgoingChat(target, literal);
  QString text = "<" + displayNick + "> " + shown;
  if (m_msgModel && sameIdentity(m_activeServer, m_activeChannel, target))
    m_msgModel->addMessage("chat", text);
  appendToChannel(m_activeServer, target, "chat", text);
}

bool IRCConnectionManager::handleSlashCommand(IrcConnection *conn,
                                               const QString &target,
                                               const QString &cmd,
                                               const QString &args) {
  if (cmd == "ECHO") { if (m_msgModel) m_msgModel->addMessage("system", args); return true; }
  const QStringList ap = args.isEmpty() ? QStringList() : args.split(' ');
  // C++ plugins take priority over Python/Lua hooks (see docs/PLUGINS.md).
  if (auto *plugins = PluginManager::instance()) {
    if (plugins->handleUserCommand(cmd.toLower(), ap, conn))
      return true;
  }
  // EAT_HEXCHAT / EAT_ALL skip the built-in command table. EAT_PLUGIN stops
  // later script engines but still falls through to the table.
  if (eatSuppressesCore(dispatchScriptCommand(cmd, ap)))
    return true;
  if (m_commandTable.isEmpty()) initCommandTable();
  auto it = m_commandTable.find(cmd);
  if (it != m_commandTable.end()) return it.value()(conn, target, args);

  // ── User-defined aliases (settings: aliases/<NAME>) ──
  if (m_settings) {
    QString expansion = m_settings->getString("aliases/" + cmd);
    if (!expansion.isEmpty()) {
      static int aliasDepth = 0;
      if (aliasDepth >= 5) {
        if (m_msgModel)
          m_msgModel->addMessage("error", "Alias recursion limit reached: /" + cmd);
        return true;
      }
      if (expansion.contains("$*"))
        expansion.replace("$*", args);
      else if (!args.isEmpty())
        expansion += " " + args;
      ++aliasDepth;
      sendMessage(target, expansion);
      --aliasDepth;
      return true;
    }
  }

  conn->sendRaw(cmd + " " + args);
  return true;
}


void IRCConnectionManager::sendRawCommand(const QString &raw) {
  if (auto *conn = activeConnection()) {
    conn->sendRaw(raw);
    emit rawLineReceived(">>", CredentialRedactor::redactIrcLine(raw));
  }
}

void IRCConnectionManager::changeNick(const QString &newNick) {
  if (auto *conn = activeConnection())
    conn->changeNick(newNick);
}

void IRCConnectionManager::setActiveServer(const QString &host) {
  m_activeServer = host;
}

void IRCConnectionManager::setActiveChannel(const QString &channel) {
  m_activeChannel = channel;
}

void IRCConnectionManager::switchToChannel(const QString &serverName,
                                           const QString &channel) {
  m_activeServer = serverName;
  m_activeChannel = channel;
  if (m_msgModel)
    m_msgModel->setBufferContext(serverName, channel);

  // Clear unread/highlight for this channel
  clearUnread(serverName, channel);

  if (!m_msgModel)
    return;

  // Tell the message model our current nick for highlight detection
  if (auto *conn = connectionForServer(serverName))
    m_msgModel->setNickname(conn->nickname());

  // Reload message history for this channel
  m_msgModel->clear();

  // Make sure we have loaded previous session's logs for this tab
  ensureScrollbackLoaded(serverName, channel);

  ChannelKey key{serverName, channel};
  if (m_history.contains(key) && !m_history[key].isEmpty()) {
    const auto &msgs = m_history[key];
    // Disable highlight for log-file scrollback, enable for session messages
    m_msgModel->setHighlightEnabled(false);
    // Batch mode: suppress per-message signals and do a single reload at the end
    m_msgModel->beginBatch();
    bool pastScrollback = false;
    for (const auto &m : msgs) {
      // Enable highlights after the scrollback end marker
      if (!pastScrollback && m.type == QLatin1String("system") &&
          m.text.contains(QLatin1String("End of scrollback"))) {
        pastScrollback = true;
        m_msgModel->addMessage(m.type, m.text, m.timestamp);
        m_msgModel->setHighlightEnabled(true);
        continue;
      }
      m_msgModel->addMessage(m.type, m.text, m.timestamp);
    }
    // If no scrollback was loaded, enable highlights now
    if (!pastScrollback)
      m_msgModel->setHighlightEnabled(true);
    m_msgModel->endBatch(); // emits reloaded() → QML reloads view in one shot
  } else {
    m_msgModel->addMessage("system", "Now talking in " + channel);
    m_msgModel->setHighlightEnabled(true);
  }

  emit currentNickChanged(currentNick());
  emit channelTopicChanged(channelTopic());
  emit channelUsersChanged(channelUsers());
  emit channelModesChanged(channelModes());
  emit awayStateChanged(isAway());
}

void IRCConnectionManager::openQuery(const QString &serverName,
                                     const QString &nick) {
  if (nick.isEmpty() || serverName.isEmpty())
    return;

  // Add to tree model if not already there
  bool isNew = false;
  if (m_treeModel && !m_treeModel->hasChannel(serverName, nick)) {
    m_treeModel->addChannel(serverName, nick);
    isNew = true;
  }

  // Switch to the query
  switchToChannel(serverName, nick);

  // Emit channelJoined so QML refreshes sidebar and selects it
  if (isNew) {
    emit channelJoined(serverName, nick);
  }
}

IrcConnection *IRCConnectionManager::activeConnection() const {
  return connectionForServer(m_activeServer);
}

QString IRCConnectionManager::currentNick() const {
  if (auto *conn = activeConnection())
    return conn->nickname();
  return "NUchat_user";
}

QString IRCConnectionManager::channelTopic() const {
  ChannelKey key{m_activeServer, m_activeChannel};
  // Render mIRC color/format codes and make URLs clickable (mIRC-style bar)
  const QString html = MessageModel::ircToHtml(m_topics.value(key));
  if (m_settings && !m_settings->getBool(QStringLiteral("url/clickable"), true))
    return html;
  return MessageModel::linkifyUrls(html);
}

QString IRCConnectionManager::channelTopicRaw() const {
  ChannelKey key{m_activeServer, m_activeChannel};
  return m_topics.value(key);
}

QString IRCConnectionManager::channelModes() const {
  ChannelKey key{m_activeServer, m_activeChannel};
  return m_modes.value(key);
}

QStringList IRCConnectionManager::channelUsers() const {
  ChannelKey key{m_activeServer, m_activeChannel};
  QStringList users = m_users.value(key);
  const IrcSupport support =
      activeConnection() ? activeConnection()->support() : IrcSupport();
  std::sort(users.begin(), users.end(), [&](const QString &a, const QString &b) {
    const int ra = a.isEmpty() ? 99 : support.symbolRank(a.at(0));
    const int rb = b.isEmpty() ? 99 : support.symbolRank(b.at(0));
    if (ra != rb)
      return ra < rb;
    const QString na = support.splitNick(a).second;
    const QString nb = support.splitNick(b).second;
    return ircCompare(na, nb, support.caseMapping()) < 0;
  });
  return users;
}

// ── Private ──

void IRCConnectionManager::wireConnection(IrcConnection *conn) {
  QString host = m_connToName[conn];

  // Forward raw lines for Raw Log + lag meter PONG detection
  connect(conn, &IrcConnection::rawLineReceived, this,
          [this, conn](const QString &line) {
            // server-time is parsed before this signal. Stamp the visible
            // model for the duration of this line, then drop it so the next
            // locally generated line uses the clock. The raw log shows the
            // redacted line; lag matching and plugins still see the original.
            if (m_msgModel && ircSame(m_activeServer, serverNameFor(conn))) {
              const QDateTime when = conn->serverTime();
              m_msgModel->setPendingTimestamp(
                  when.isValid() ? when.toLocalTime().toString(Qt::ISODate)
                                 : QString());
              QTimer::singleShot(0, this, [this] {
                if (m_msgModel)
                  m_msgModel->setPendingTimestamp({});
              });
            }
            const QString shown = CredentialRedactor::redactIrcLine(line);
            emit rawLineReceived("<<", shown);
            if (m_settings) {
              bool showRaw = false;
              if (m_settings->contains(QStringLiteral("adv/showRawIRC")))
                showRaw = m_settings->getBool(QStringLiteral("adv/showRawIRC"), false);
              else if (m_settings->contains(QStringLiteral("adv/showRaw")))
                showRaw = m_settings->getBool(QStringLiteral("adv/showRaw"), false);
              if (showRaw) {
                const QString srv = serverNameFor(conn);
                const QString text = QStringLiteral("<< ") + shown;
                appendToChannel(srv, srv, QStringLiteral("system"), text);
                if (m_msgModel && isViewing(srv, srv))
                  m_msgModel->addMessage(QStringLiteral("system"), text);
              }
            }
            // Detect PONG response to our LAG ping — match the actual PONG
            // command with our LAG token as final param, not any chat line
            // that merely mentions both words
            static const QRegularExpression lagPongRe(
                QStringLiteral("^(?::\\S+ )?PONG (?:\\S+ )*:?LAG\\d+$"));
            if (m_lagState.contains(conn) && m_lagState[conn].pending &&
                lagPongRe.match(line).hasMatch()) {
              auto &state = m_lagState[conn];
              state.lagMs = static_cast<int>(state.pingSent.elapsed());
              state.pending = false;
              // Report the active connection's lag
              if (conn == activeConnection()) {
                m_lagMs = state.lagMs;
                emit lagChanged(m_lagMs);
              }
            }
          });

  // Registered (001)
  connect(conn, &IrcConnection::registered, this, [this, conn]() {
    QString srv = serverNameFor(conn);
    QString msg = "Connected to " + srv + " as " + conn->nickname();
    appendToChannel(srv, srv, "system", msg);
    if (m_msgModel && ircSame(m_activeServer, srv)) {
      m_msgModel->addMessage("system", msg);
    }
    emit serverRegistered(srv);
    emit currentNickChanged(conn->nickname());

    // Cancel any pending reconnect timer for this server
    if (m_reconnectInfo.contains(srv) && m_reconnectInfo[srv].timer) {
      m_reconnectInfo[srv].timer->stop();
      m_reconnectInfo[srv].timer->deleteLater();
      m_reconnectInfo[srv].timer = nullptr;
      m_reconnectInfo[srv].attempts = 0;
    }

    // ── Execute perform-on-connect commands ──
    executePerformCommands(srv);

    // ── Per-network perform commands + autojoin (network list / HexChat import) ──
    if (m_reconnectInfo.contains(srv)) {
      const ReconnectInfo &ri = m_reconnectInfo[srv];
      const QStringList performLines =
          ri.perform.split('\n', Qt::SkipEmptyParts);
      for (const QString &line : performLines) {
        QString cmd = line.trimmed();
        if (cmd.isEmpty()) continue;
        if (cmd.startsWith('/')) {
          QString prevServer = m_activeServer, prevChannel = m_activeChannel;
          m_activeServer = srv;
          m_activeChannel = srv;
          sendMessage(srv, cmd);
          m_activeServer = prevServer;
          m_activeChannel = prevChannel;
        } else {
          conn->sendRaw(cmd);
        }
      }
      static const QRegularExpression chanSep(QStringLiteral("[,\\s]+"));
      const QStringList autojoinChans =
          ri.autojoin.split(chanSep, Qt::SkipEmptyParts);
      for (const QString &chan : autojoinChans) {
        if (conn->support().isChannel(chan))
          conn->joinChannel(chan);
      }
    }

    // ── Notify list: subscribe via IRCv3 MONITOR ──
    if (m_settings) {
      QStringList notifyList =
          m_settings->value("notify/list").toStringList();
      if (!notifyList.isEmpty())
        conn->sendRaw("MONITOR + " + notifyList.join(','));
    }

    // ── Start lag meter ──
    if (!m_lagTimer.isActive()) m_lagTimer.start();
  });

  // Disconnected
  connect(conn, &IrcConnection::disconnectedFromServer, this, [this, conn]() {
    QString srv = serverNameFor(conn);
    QString msg = "Disconnected from " + srv;
    appendToChannel(srv, srv, "system", msg);
    if (m_msgModel && ircSame(m_activeServer, srv)) {
      m_msgModel->addMessage("system", msg);
    }

    // ── Auto-reconnect ──
    // Only suppress reconnect if THIS host was intentionally disconnected
    if (!m_userDisconnected.remove(srv)) {
      attemptReconnect(srv);
    }

    // Clean up lag state for this connection
    m_lagState.remove(conn);

    // ── Stop lag meter if no live connections left ──
    // (disconnected connections can linger in m_connections until the user
    // closes the tab or a reconnect replaces them, so count actual state)
    bool anyConnected = false;
    for (auto *c : m_connections) {
      if (c && c->isConnected()) {
        anyConnected = true;
        break;
      }
    }
    if (!anyConnected) {
      m_lagTimer.stop();
      m_lagMs = -1;
      emit lagChanged(m_lagMs);
    }
  });

  // Error
  connect(conn, &IrcConnection::errorOccurred, this,
          [this, conn](const QString &err) {
            QString srv = serverNameFor(conn);
            appendToChannel(srv, srv, "error", err);
            if (m_msgModel && ircSame(m_activeServer, srv))
              m_msgModel->addMessage("error", err);
          });

  // PRIVMSG
  connect(conn, &IrcConnection::privmsgReceived, this,
          [this, conn](const QString &prefix, const QString &target,
                       const QString &message) {
            QString srv = serverNameFor(conn);
            QString nick =
                prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;

            // ── Ignore list filtering ──
            if (isIgnored(prefix) || isIgnored(nick))
              return;

            QString channel = target;
            const bool fromSelf = conn->support().same(nick, conn->nickname());
            // If target is our nick, it's a PM — use sender's nick as channel
            // (IRC targets are case-insensitive)
            bool isQuery = false;
            if (conn->support().same(target, conn->nickname())) {
              channel = nick;
              isQuery = true;
            } else if (fromSelf && !conn->support().isChannel(target)) {
              // A PM we sent from another client, relayed by ZNC
              // (znc.in/self-message): file it under the recipient.
              isQuery = true;
            }
            // Auto-create query tab for PMs
            if (isQuery && m_treeModel && !m_treeModel->hasChannel(srv, channel)) {
              m_treeModel->addChannel(srv, channel);
              emit channelAdded(srv, channel);
            }

            // Look up user's channel prefix (@, +, etc.)
            QString displayNick = nick;
            ChannelKey key{srv, channel};
            if (m_users.contains(key)) {
              for (const QString &u : m_users[key]) {
                auto [pfx, bare] = conn->support().splitNick(u);
                if (conn->support().same(bare, nick)) {
                  if (!pfx.isEmpty())
                    displayNick = pfx[0] + nick;
                  break;
                }
              }
            }

            QString text = "<" + displayNick + "> " + message;
            m_appendingOwnLine = fromSelf;
            appendToChannel(srv, channel, "chat", text);
            m_appendingOwnLine = false;
            if (m_msgModel && isViewing(srv, channel)) {
              m_msgModel->addMessage("chat", text);
            }
          });

  // NOTICE
  connect(conn, &IrcConnection::noticeReceived, this,
          [this, conn](const QString &prefix, const QString &target,
                       const QString &message) {
            QString srv = serverNameFor(conn);
            QString nick =
                prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;

            // ── Ignore list filtering ──
            if (isIgnored(prefix) || isIgnored(nick))
              return;

            QString text = "-" + nick + "- " + message;
            // Route to the target channel if it's a channel, to query if one
            // exists, otherwise server tab
            QString dest;
            if (conn->support().isChannel(target)) {
              dest = target;
            } else if (m_treeModel && m_treeModel->hasChannel(srv, nick)) {
              // Route to existing query window for this user
              dest = nick;
            } else {
              dest = srv;
            }
            appendToChannel(srv, dest, "system", text);
            if (m_msgModel && isViewing(srv, dest)) {
              m_msgModel->addMessage("system", text);
            } else if (dest != srv) {
              // Mark channel/query tabs unread for notices (server tab excluded
              // to avoid noise from services/server notices)
              QString ukey = unreadKey(srv, dest);
              if (!m_unread.contains(ukey)) {
                m_unread.insert(ukey);
                emit unreadStateChanged();
              }
            }
          });

  // JOIN
  connect(
      conn, &IrcConnection::joinReceived, this,
      [this, conn](const QString &prefix, const QString &channel) {
        QString srv = serverNameFor(conn);
        QString nick =
            prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        if (sameIdentity(srv, nick, conn->nickname())) {
          // We joined — add channel to tree
          if (m_treeModel) {
            m_treeModel->addChannel(srv, channel);
          }
          emit channelJoined(srv, channel);

          appendToChannel(srv, channel, "system", "Now talking in " + channel);
          // Auto-switch to the new channel (server too — the join may have
          // happened on a non-active connection, e.g. auto-join on reconnect).
          // switchToChannel() loads log scrollback exactly once (via
          // ensureScrollbackLoaded) and replays the history into the model in
          // a single batch; loading the log again here as well would display
          // the scrollback duplicated.
          // "Open tabs in background" keeps the user on the buffer they
          // were already reading.
          const bool background =
              m_settings &&
              m_settings->getBool(QStringLiteral("ui/tabsInBackground"), false);
          if (!background)
            switchToChannel(srv, channel);
          // Request channel modes so we can display them
          conn->sendRaw("MODE " + channel);
        } else {
          QString text = nick + " has joined " + channel;
          appendToChannel(srv, channel, "system", text);
          if (m_msgModel && isViewing(srv, channel)) {
            m_msgModel->addMessage("system", text);
          }
          // Add to user list
          ChannelKey key{srv, channel};
          if (m_users.contains(key) &&
              std::none_of(m_users[key].cbegin(), m_users[key].cend(),
                           [&](const QString &u) {
                             return conn->support().same(
                                 conn->support().splitNick(u).second, nick);
                           })) {
            m_users[key].append(nick);
            if (isViewing(srv, channel))
              emit channelUsersChanged(channelUsers());
          }
        }
      });

  // PART
  connect(
      conn, &IrcConnection::partReceived, this,
      [this, conn](const QString &prefix, const QString &channel,
                   const QString &reason) {
        QString srv = serverNameFor(conn);
        QString nick =
            prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = nick + " has left " + channel;
        if (!reason.isEmpty())
          text += " (" + reason + ")";
        appendToChannel(srv, channel, "system", text);
        if (m_msgModel && isViewing(srv, channel)) {
          m_msgModel->addMessage("system", text);
        }
        if (sameIdentity(srv, nick, conn->nickname())) {
          emit channelParted(srv, channel);
        }
        // Remove from user list
        ChannelKey key{srv, channel};
        if (m_users.contains(key)) {
          auto &users = m_users[key];
          users.erase(std::remove_if(
                          users.begin(), users.end(),
                          [&](const QString &u) {
                            auto [pfx, bare] = conn->support().splitNick(u);
                            return conn->support().same(bare, nick);
                          }),
                      users.end());
          if (isViewing(srv, channel))
            emit channelUsersChanged(channelUsers());
        }
      });

  // QUIT
  connect(
      conn, &IrcConnection::quitReceived, this,
      [this, conn](const QString &prefix, const QString &reason) {
        QString srv = serverNameFor(conn);
        QString nick =
            prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = nick + " has quit";
        if (!reason.isEmpty())
          text += " (" + reason + ")";
        // Remove from all channels on this server and show quit message
        bool emitUpdate = false;
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
          if (!ircSame(it.key().server, srv))
            continue;
          auto &users = it.value();
          int before = users.size();
          users.erase(std::remove_if(
                          users.begin(), users.end(),
                          [&](const QString &u) {
                            auto [pfx, bare] = conn->support().splitNick(u);
                            return conn->support().same(bare, nick);
                          }),
                      users.end());
          if (users.size() != before) {
            // User was in this channel — show quit message there
            appendToChannel(srv, it.key().channel, "system", text);
            if (isViewing(srv, it.key().channel))
              emitUpdate = true;
          }
        }
        if (emitUpdate) {
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          emit channelUsersChanged(channelUsers());
        }
      });

  // KICK
  connect(
      conn, &IrcConnection::kickReceived, this,
      [this, conn](const QString &prefix, const QString &channel,
                   const QString &kicked, const QString &reason) {
        QString srv = serverNameFor(conn);
        QString kicker =
            prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = kicked + " was kicked by " + kicker;
        if (!reason.isEmpty())
          text += " (" + reason + ")";
        appendToChannel(srv, channel, "system", text);
        if (m_msgModel && isViewing(srv, channel)) {
          m_msgModel->addMessage("system", text);
        }
        // Remove kicked user from list
        ChannelKey key{srv, channel};
        if (m_users.contains(key)) {
          auto &users = m_users[key];
          users.erase(
              std::remove_if(users.begin(), users.end(),
                             [&](const QString &u) {
                               auto [pfx, bare] = conn->support().splitNick(u);
                               return conn->support().same(bare, kicked);
                             }),
              users.end());
          if (isViewing(srv, channel))
            emit channelUsersChanged(channelUsers());
        }
      });

  // NICK change
  connect(conn, &IrcConnection::nickChanged, this,
          [this, conn](const QString &oldNick, const QString &newNick) {
            QString srv = serverNameFor(conn);
            QString text = oldNick + " is now known as " + newNick;
            if (sameIdentity(srv, oldNick, conn->nickname()) ||
                sameIdentity(srv, newNick, conn->nickname())) {
              emit currentNickChanged(conn->nickname());
            }
            // Update nick in all channel user lists on this server
            // and show the nick-change message only in channels where the user
            // is present
            bool emitUpdate = false;
            for (auto it = m_users.begin(); it != m_users.end(); ++it) {
              if (!ircSame(it.key().server, srv))
                continue;
              auto &users = it.value();
              for (int i = 0; i < users.size(); i++) {
                QString entry = users[i];
                auto [prefix, bare] = conn->support().splitNick(entry);
                if (conn->support().same(bare, oldNick)) {
                  users[i] = prefix + newNick;
                  appendToChannel(srv, it.key().channel, "system", text);
                  if (isViewing(srv, it.key().channel)) {
                    emitUpdate = true;
                    if (m_msgModel)
                      m_msgModel->addMessage("system", text);
                  }
                  break;
                }
              }
            }
            if (emitUpdate)
              emit channelUsersChanged(channelUsers());
          });

  // TOPIC
  connect(conn, &IrcConnection::topicReceived, this,
          [this, conn](const QString &channel, const QString &topic) {
            QString srv = serverNameFor(conn);
            ChannelKey key{srv, channel};
            m_topics[key] = topic;
            QString text = "Topic for " + channel + ": " + topic;
            appendToChannel(srv, channel, "system", text);
            if (m_msgModel && isViewing(srv, channel)) {
              m_msgModel->addMessage("system", text);
              emit channelTopicChanged(topic);
            }
          });

  // MODE
  connect(
      conn, &IrcConnection::modeReceived, this,
      [this, conn](const QString &prefix, const QString &target,
                   const QString &modeStr, const QStringList &params) {
        QString srv = serverNameFor(conn);
        QString nick =
            prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = nick + " sets mode " + modeStr;
        if (!params.isEmpty())
          text += " " + params.join(" ");
        text += " on " + target;
        const IrcSupport &support = conn->support();
        QString dest = support.isChannel(target) ? target : srv;
        appendToChannel(srv, dest, "system", text);
        if (m_msgModel && isViewing(srv, dest)) {
          m_msgModel->addMessage("system", text);
        }

        // Update user prefixes in the nick list for channel modes
        if (support.isChannel(target)) {
          ChannelKey key{srv, target};
          if (!m_users.contains(key))
            return;

          bool adding = true;
          int paramIdx = 0;
          QStringList &users = m_users[key];

          for (QChar c : modeStr) {
            if (c == '+') {
              adding = true;
              continue;
            }
            if (c == '-') {
              adding = false;
              continue;
            }

            QChar pfx = support.prefixForMode(c);
            if (pfx.isNull()) {
              // Non-user channel mode — keep the parameter index aligned
              // using this server's CHANMODES (defaults until ISUPPORT).
              if (support.modeTakesParam(c, adding))
                paramIdx++;
              continue;
            }
            if (paramIdx >= params.size())
              continue;

            QString affected = params[paramIdx++];

            // Find and update the user in the list
            for (int i = 0; i < users.size(); i++) {
              const auto split = support.splitNick(users[i]);
              QString currentPrefix = split.first;
              const QString bareNick = split.second;
              if (!support.same(bareNick, affected))
                continue;

              if (adding) {
                if (!currentPrefix.contains(pfx))
                  currentPrefix += pfx;
              } else {
                currentPrefix.remove(pfx);
              }

              // Highest-rank prefix only, in this server's PREFIX order.
              QChar best;
              int bestRank = 99;
              for (QChar p : currentPrefix) {
                const int r = support.symbolRank(p);
                if (r < bestRank) {
                  bestRank = r;
                  best = p;
                }
              }
              users[i] = best.isNull() ? bareNick : (QString(best) + bareNick);
              break;
            }
          }

          // Re-emit sorted list if active channel
          if (isViewing(srv, target)) {
            emit channelUsersChanged(channelUsers());
          }
        }
      });

  // CTCP
  connect(conn, &IrcConnection::ctcpReceived, this,
          [this, conn](const QString &prefix, const QString &target,
                       const QString &command, const QString &args) {
            QString srv = serverNameFor(conn);
            QString nick =
                prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;

            // ── Ignore list filtering ──
            if (isIgnored(prefix) || isIgnored(nick))
              return;

            // Determine the proper destination channel
            // (IRC targets are case-insensitive)
            QString channel = target;
            const bool fromSelf = conn->support().same(nick, conn->nickname());
            if (conn->support().same(target, conn->nickname())) {
              channel = nick; // PM
            }
            QString text;
            if (command == "ACTION") {
              // /me we sent to someone from another client (ZNC self-message)
              if (fromSelf && !conn->support().isChannel(target) &&
                  m_treeModel && !m_treeModel->hasChannel(srv, channel)) {
                m_treeModel->addChannel(srv, channel);
                emit channelAdded(srv, channel);
              }
              text = "* " + nick + " " + args;
              m_appendingOwnLine = fromSelf;
              appendToChannel(srv, channel, "action", text);
              m_appendingOwnLine = false;
              if (m_msgModel && isViewing(srv, channel)) {
                m_msgModel->addMessage("action", text);
              }
            } else if (command == "DCC" && m_dccManager) {
              // ── DCC SEND parsing ──
              // Format: DCC SEND <filename> <ip> <port> <filesize> [token]
              // Filename may be quoted: DCC SEND "my file.txt" 123456 1024 5000
              QString rest = args.trimmed();
              if (rest.startsWith("SEND ", Qt::CaseInsensitive)) {
                rest = rest.mid(5).trimmed();
                QString fileName;
                if (rest.startsWith('"')) {
                  int closeQuote = rest.indexOf('"', 1);
                  if (closeQuote > 0) {
                    fileName = rest.mid(1, closeQuote - 1);
                    rest = rest.mid(closeQuote + 1).trimmed();
                  }
                } else {
                  fileName = rest.section(' ', 0, 0);
                  rest = rest.section(' ', 1).trimmed();
                }
                QStringList parts = rest.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 3) {
                  quint32 ip = parts[0].toUInt();
                  quint16 port = parts[1].toUShort();
                  qint64 size = parts[2].toLongLong();
                  int token = parts.size() > 3 ? parts[3].toInt() : 0;
                  refreshDccSettings();
                  m_dccManager->setConnection(conn);
                  m_dccManager->handleDccSend(nick, fileName, ip, port, size, token);
                  // Show in server buffer
                  text = "DCC SEND offer from " + nick + ": " + fileName +
                         " (" + DccTransfer::formatSize(size) + ")";
                  appendToChannel(srv, srv, "system", text);
                  if (m_msgModel && isViewing(srv, srv))
                    m_msgModel->addMessage("system", text);
                }
              }
            } else {
              text = "CTCP " + command + " from " + nick +
                     (args.isEmpty() ? "" : ": " + args);
              appendToChannel(srv, srv, "system", text);
              if (m_msgModel && isViewing(srv, srv))
                m_msgModel->addMessage("system", text);
            }
          });

  // NAMES list
  connect(conn, &IrcConnection::namesReceived, this,
          [this, conn](const QString &channel, const QStringList &names) {
            QString srv = serverNameFor(conn);
            ChannelKey key{srv, channel};
            m_users[key] = names;
            // Emit if this is the active channel
            if (isViewing(srv, channel)) {
              emit channelUsersChanged(channelUsers());
            }
          });

  // Numeric replies (general — for MOTD etc.)
  connect(
      conn, &IrcConnection::numericReceived, this,
      [this, conn](int code, const QStringList &params,
                   const QString &trailing) {
        QString srv = serverNameFor(conn);
        // A replaced connection is no longer in the map. Drop the event
        // instead of filing it under a fake "unknown" server.
        if (srv.isEmpty())
          return;

        if (code == 5)
          publishSupport(srv, conn->support());

        // ── WHOIS replies (311–319, 330, 338, 378, 671) ──
        // Accumulate all WHOIS lines into a buffer, then display as a
        // structured block when RPL_ENDOFWHOIS (318) arrives.
        if (code == 311) {
          // RPL_WHOISUSER: <nick> <user> <host> * :<realname>
          QString nick = params.value(0);
          QString user = params.value(1);
          QString host = params.value(2);
          QString key = srv + "\n" + nick.toLower();
          if (m_whoisBuffer.size() > 50)
            m_whoisBuffer.erase(m_whoisBuffer.begin());
          m_whoisBuffer[key].clear(); // start fresh
          m_whoisBuffer[key] << nick + " (" + user + "@" + host + ")";
          m_whoisBuffer[key] << "  Real name: " + trailing;
        } else if (code == 312) {
          // RPL_WHOISSERVER: <nick> <server> :<server info>
          QString nick = params.value(0);
          QString server = params.value(1);
          QString key = srv + "\n" + nick.toLower();
          m_whoisBuffer[key] << "  Server: " + server + " (" + trailing + ")";
        } else if (code == 313) {
          // RPL_WHOISOPERATOR
          QString nick = params.value(0);
          QString key = srv + "\n" + nick.toLower();
          m_whoisBuffer[key] << "  " + trailing;
        } else if (code == 317) {
          // RPL_WHOISIDLE: <nick> <seconds> <signon> :seconds idle
          QString nick = params.value(0);
          int idleSecs = params.value(1).toInt();
          int d = idleSecs / 86400, h = (idleSecs % 86400) / 3600,
              m = (idleSecs % 3600) / 60, s = idleSecs % 60;
          QString idle;
          if (d > 0)
            idle += QString::number(d) + "d ";
          if (h > 0)
            idle += QString::number(h) + "h ";
          if (m > 0)
            idle += QString::number(m) + "m ";
          idle += QString::number(s) + "s";
          QString signonTime;
          if (params.size() > 2) {
            qint64 ts = params.value(2).toLongLong();
            signonTime = QDateTime::fromSecsSinceEpoch(ts).toString(
                "yyyy-MM-dd hh:mm:ss");
          }
          QString key = srv + "\n" + nick.toLower();
          QString line = "  Idle: " + idle;
          if (!signonTime.isEmpty())
            line += " | Signed on: " + signonTime;
          m_whoisBuffer[key] << line;
        } else if (code == 318) {
          // RPL_ENDOFWHOIS — flush the accumulated WHOIS block onto this
          // server's own buffer, never another server's active channel.
          QString nick = params.value(0);
          QString key = srv + "\n" + nick.toLower();

          if (m_whoisBuffer.contains(key) && !m_whoisBuffer[key].isEmpty()) {
            QString sep = "━━━━━━━━ WHOIS " + nick + " ━━━━━━━━";
            showOnOriginServer(srv, "system", sep);
            for (const QString &line : m_whoisBuffer[key])
              showOnOriginServer(srv, "system", line);
            showOnOriginServer(srv, "system",
                               "━━━━━━━━ End of WHOIS ━━━━━━━━");
            m_whoisBuffer.remove(key);
          } else {
            showOnOriginServer(srv, "system",
                               "[WHOIS] No information for " + nick);
          }
        } else if (code == 319) {
          // RPL_WHOISCHANNELS: <nick> :<channels>
          QString nick = params.value(0);
          QString key = srv + "\n" + nick.toLower();
          m_whoisBuffer[key] << "  Channels: " + trailing;
        } else if (code == 330) {
          // RPL_WHOISACCOUNT: <nick> <account> :is logged in as
          QString nick = params.value(0);
          QString acct = params.value(1);
          QString key = srv + "\n" + nick.toLower();
          m_whoisBuffer[key] << "  Account: " + acct;
        } else if (code == 338) {
          // RPL_WHOISACTUALLY: <nick> <ip> :actually using host
          QString nick = params.value(0);
          QString ip = params.value(1);
          QString key = srv + "\n" + nick.toLower();
          m_whoisBuffer[key] << "  Actual host: " + ip;
        } else if (code == 378) {
          // RPL_WHOISHOST: connecting from
          QString nick = params.value(0);
          QString key = srv + "\n" + nick.toLower();
          m_whoisBuffer[key] << "  " + trailing;
        } else if (code == 671) {
          // RPL_WHOISSECURE
          QString nick = params.value(0);
          QString key = srv + "\n" + nick.toLower();
          m_whoisBuffer[key] << "  " + trailing;
        } else if (code == 324) {
          // RPL_CHANNELMODEIS: <channel> <modes> [<mode params>]
          QString channel = params.value(1);
          QString modes = params.value(2);
          QStringList modeParams = params.mid(3);
          QString text = "Channel modes for " + channel + ": " + modes;
          if (!modeParams.isEmpty())
            text += " " + modeParams.join(" ");
          // Store the mode string for display in the topic bar
          ChannelKey key{srv, channel};
          m_modes[key] = modes;
          if (isViewing(srv, channel))
            emit channelModesChanged(modes);
          appendToChannel(srv, channel, "system", text);
          if (m_msgModel && isViewing(srv, channel))
            m_msgModel->addMessage("system", text);
        } else if (code == 329) {
          // RPL_CREATIONTIME: <channel> <timestamp>
          QString channel = params.value(1);
          qint64 ts = params.value(2).toLongLong();
          QString timeStr = QDateTime::fromSecsSinceEpoch(ts).toString(
              "ddd MMM d hh:mm:ss yyyy");
          QString text = "Channel " + channel + " created on " + timeStr;
          appendToChannel(srv, channel, "system", text);
          if (m_msgModel && isViewing(srv, channel))
            m_msgModel->addMessage("system", text);
        } else if (code == 314) {
          // RPL_WHOWASUSER
          QString nick = params.value(1);
          QString user = params.value(2);
          QString host = params.value(3);
          QString text = "[WHOWAS] " + nick + " was " + user + "@" + host +
                         " : " + trailing;
          showOnOriginServer(srv, "system", text);
        }

        // RPL_UNAWAY (305) and RPL_NOWAWAY (306) — per-server away state
        else if (code == 305) {
          const int logged = m_awayByServer.value(srv).log.size();
          setServerAway(srv, false, false);
          QString text = trailing.isEmpty()
                             ? "You are no longer marked as being away"
                             : trailing;
          showOnOriginServer(srv, "system", text);
          if (logged > 0) {
            showOnOriginServer(
                srv, "system",
                "You have " + QString::number(logged) +
                    " message(s) in your away log. Use View > Away "
                    "Log to see them.");
          }
        } else if (code == 306) {
          setServerAway(srv, true, true);
          QString text = trailing.isEmpty()
                             ? "You have been marked as being away"
                             : trailing;
          showOnOriginServer(srv, "system", text);
        }

        // RPL_AWAY (301) — user is away
        else if (code == 301) {
          QString nick = params.value(0);
          QString key = srv + "\n" + nick.toLower();
          if (m_whoisBuffer.contains(key)) {
            m_whoisBuffer[key] << "  Away: " + trailing;
          } else {
            showOnOriginServer(srv, "system", nick + " is away: " + trailing);
          }
        }

        // ── RPL_LIST (322): <channel> <# visible> :<topic> ──
        else if (code == 322) {
          QString channel = params.value(0);
          int users = params.value(1).toInt();
          emit channelListEntry(channel, users, trailing);
        }
        // ── RPL_LISTEND (323) ──
        else if (code == 323) {
          emit channelListEnd();
        }

        // ── MONITOR notify list: RPL_MONONLINE (730) / RPL_MONOFFLINE (731) ──
        else if (code == 730 || code == 731) {
          // trailing: comma-separated nick[!user@host] list
          const QStringList targets = trailing.split(',', Qt::SkipEmptyParts);
          for (const QString &t : targets) {
            QString nick = t.section('!', 0, 0).trimmed();
            if (nick.isEmpty()) continue;
            bool online = (code == 730);
            QString text = nick + (online
                ? QStringLiteral(" is online (notify)")
                : QStringLiteral(" is offline (notify)"));
            appendToChannel(srv, srv, "system", text);
            if (m_msgModel && ircSame(m_activeServer, srv))
              m_msgModel->addMessage("system", text);
            if (online)
              emit notifyUser(QStringLiteral("Notify"), text, false, false);
          }
        }

        // ── RPL_BANLIST (367): <channel> <mask> <setter> <timestamp> ──
        else if (code == 367) {
          // params[0]=channel, [1]=mask, [2]=setter, [3]=timestamp
          // (our nick was already stripped by IrcConnection)
          QString channel = params.value(0);
          QString mask = params.value(1);
          QString setter = params.value(2);
          // The timestamp is a Unix epoch; convert to readable
          QString timeStr;
          if (params.size() > 3) {
            qint64 ts = params.value(3).toLongLong();
            timeStr = QDateTime::fromSecsSinceEpoch(ts).toString(
                "yyyy-MM-dd hh:mm");
          }
          emit banListEntry(channel, mask, setter, timeStr);
        }
        // ── RPL_ENDOFBANLIST (368) ──
        else if (code == 368) {
          QString channel = params.value(0);
          emit banListEnd(channel);
        }
        else if ((code >= 372 && code <= 376) || code == 2 || code == 3 ||
                 code == 4 || code == 5 || code == 251 || code == 252 ||
                 code == 253 || code == 254 || code == 255 || code == 265 ||
                 code == 266) {
          appendToChannel(srv, srv, "system", trailing);
          if (ircSame(m_activeServer, srv) &&
              (m_activeChannel.isEmpty() || ircSame(m_activeChannel, srv))) {
            if (m_msgModel)
              m_msgModel->addMessage("system", trailing);
          }
        }
        // Error numerics — route to the relevant channel if mentioned in
        // params, else server tab
        if (code >= 400 && code < 600) {
          // Clear any pending WHOIS buffer for ERR_NOSUCHNICK (401)
          if (code == 401 && !params.isEmpty()) {
            QString failedNick = params.value(0);
            m_whoisBuffer.remove(srv + "\n" +
                                 conn->support().fold(failedNick));
          }
          QString text = "[" + QString::number(code) + "] " + trailing;
          // Some error numerics mention a channel in params (e.g. 404 #channel
          // :Cannot send)
          QString dest = srv;
          for (int pi = 1; pi < params.size(); pi++) {
            if (conn->support().isChannel(params[pi])) {
              dest = params[pi];
              break;
            }
          }
          appendToChannel(srv, dest, "error", text);
          if (m_msgModel && isViewing(srv, dest))
            m_msgModel->addMessage("error", text);
        }
      });
}

void IRCConnectionManager::appendToChannel(const QString &server,
                                           const QString &channel,
                                           const QString &type,
                                           const QString &text) {
  if (server.isEmpty() || channel.isEmpty())
    return;

  // Make sure scrollback is populated into history before we start appending
  // new messages
  ensureScrollbackLoaded(server, channel);

  ChannelKey key{server, channel};
  StoredMessage msg;
  msg.type = type;
  msg.text = text;
  msg.timestamp = messageTimestamp(server);
  auto &hist = m_history[key];
  hist.append(msg);
  trimHistory(hist);

  // Log to file
  if (m_logger)
    m_logger->log(server, channel, type, text);

  // Track unread state for non-active channels
  bool isActive = isViewing(server, channel);
  if (!isActive && !m_appendingOwnLine && (type == "chat" || type == "action")) {
    QString ukey = unreadKey(server, channel);
    bool changed = false;
    bool isNickHighlight = false;
    bool isPrivateMsg = false;

    if (!m_unread.contains(ukey)) {
      m_unread.insert(ukey);
      changed = true;
    }
    // Check for nick highlight (whole-word mention of our nick)
    if (auto *conn = connectionForServer(server)) {
      QString myNick = conn->nickname();
      if (!myNick.isEmpty() && MessageModel::containsNickWord(text, myNick)) {
        isNickHighlight = true;
        if (!m_highlighted.contains(ukey)) {
          m_highlighted.insert(ukey);
          changed = true;
        }
      }
    }
    // Also mark all PMs (non-channel targets) as highlighted
    if (!isChannelName(server, channel) && channel != server) {
      isPrivateMsg = true;
      if (!m_highlighted.contains(ukey)) {
        m_highlighted.insert(ukey);
        changed = true;
      }
    }
    if (changed)
      emit unreadStateChanged();

    // Fire desktop notification for highlights and PMs
    if (isNickHighlight || isPrivateMsg) {
      QString title = isPrivateMsg
          ? QStringLiteral("PM from ") + channel
          : QStringLiteral("Highlight in ") + channel;
      // Strip formatting from the message body for notification text
      QString body = text;
      // Remove "<nick> " prefix for cleaner notification
      if (body.startsWith('<')) {
        int gt = body.indexOf('>');
        if (gt > 0) body = body.mid(gt + 2);
      }
      emit notifyUser(title, body, isNickHighlight, isPrivateMsg);
    }
  }

  // ── URL Grabber — extract URLs from chat/action messages ──
  if (type == "chat" || type == "action") {
    // Extract the nick from the message text
    QString urlNick;
    if (type == "chat" && text.startsWith('<')) {
      urlNick = text.section('>', 0, 0).mid(1);
      // Strip mode prefix from nick using this server's PREFIX symbols.
      const QString symbols = IrcNetworkRegistry::factsFor(server).prefixSymbols;
      while (!urlNick.isEmpty() && symbols.contains(urlNick[0]))
        urlNick = urlNick.mid(1);
    } else if (type == "action" && text.startsWith("* ")) {
      urlNick = text.section(' ', 1, 1);
    }
    extractUrls(text, urlNick, channel);

    // ── Away log — only for the server that is actually away ──
    auto awayIt = m_awayByServer.find(server);
    if (awayIt != m_awayByServer.end() && awayIt->away) {
      AwayLogEntry entry;
      const QString inbound = messageTimestamp(server);
      const QDateTime parsed = QDateTime::fromString(inbound, Qt::ISODate);
      entry.timestamp = (parsed.isValid() ? parsed : QDateTime::currentDateTime())
                            .toString(QStringLiteral("hh:mm:ss"));
      entry.nick = urlNick;
      entry.channel = channel;
      entry.message = text;
      awayIt->log.append(entry);
      if (awayIt->log.size() > kMaxAwayLogEntries) {
        int excess = awayIt->log.size() - kMaxAwayLogEntries;
        awayIt->log.remove(0, excess);
      }
      if (ircSame(m_activeServer, server))
        emit awayLogUpdated();
    }
  }
}

void IRCConnectionManager::ensureScrollbackLoaded(const QString &server,
                                                  const QString &channel) {
  QString ukey = unreadKey(server, channel);
  if (m_scrollbackLoaded.contains(ukey)) {
    return;
  }
  m_scrollbackLoaded.insert(ukey);

  if (!m_logger) {
    return;
  }

  auto entries = m_logger->loadScrollback(server, channel, 200);
  if (entries.isEmpty()) {
    return;
  }

  ChannelKey key{server, channel};

  // We want to insert the scrollback BEFORE any existing messages in m_history
  // for this session.
  QVector<StoredMessage> prependedHistory;

  StoredMessage startHeader;
  startHeader.type = "system";
  startHeader.text = "── Scrollback from " + channel + " ──";
  startHeader.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
  prependedHistory.append(startHeader);

  for (const auto &e : entries) {
    StoredMessage sm;
    sm.type = e.type;
    sm.text = e.text;
    sm.timestamp = e.timestamp;
    prependedHistory.append(sm);
  }

  StoredMessage endHeader;
  endHeader.type = "system";
  endHeader.text = "── End of scrollback ──";
  endHeader.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
  prependedHistory.append(endHeader);

  // If we already received some elements in this session (should be rare since
  // we call this on the first append), append those afterward.
  if (m_history.contains(key)) {
    prependedHistory.append(m_history[key]);
  }

  m_history[key] = prependedHistory;
}

void IRCConnectionManager::cleanupChannelState(const QString &server,
                                                const QString &channel) {
  ChannelKey key{server, channel};
  m_history.remove(key);
  m_users.remove(key);
  m_topics.remove(key);
  m_modes.remove(key);
  // Also remove the scrollback-loaded marker so it reloads if re-joined
  m_scrollbackLoaded.remove(unreadKey(server, channel));
}

QString IRCConnectionManager::serverNameFor(IrcConnection *conn) const {
  return m_connToName.value(conn);
}

bool IRCConnectionManager::hasUnread(const QString &server,
                                     const QString &channel) const {
  return m_unread.contains(unreadKey(server, channel));
}

bool IRCConnectionManager::hasHighlight(const QString &server,
                                        const QString &channel) const {
  return m_highlighted.contains(unreadKey(server, channel));
}

void IRCConnectionManager::clearUnread(const QString &server,
                                       const QString &channel) {
  QString ukey = unreadKey(server, channel);
  bool changed = false;
  if (m_unread.remove(ukey))
    changed = true;
  if (m_highlighted.remove(ukey))
    changed = true;
  if (changed)
    emit unreadStateChanged();
}

// ── /SYSINFO helper ──
static QString readFileFirstLine(const QString &path) {
  QFile f(path);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return QTextStream(&f).readLine().trimmed();
  }
  return {};
}

static QString runProcess(const QString &program, const QStringList &args,
                          int timeout = 2000) {
  QProcess proc;
  proc.start(program, args);
  if (proc.waitForFinished(timeout))
    return proc.readAllStandardOutput().trimmed();
  // Timed out — kill it so we don't leave a stray child process behind
  proc.kill();
  proc.waitForFinished(500);
  return {};
}

QString IRCConnectionManager::gatherSysInfo() {
  QStringList parts;

  // OS
  QString prettyName = readFileFirstLine("/etc/os-release");
  // Parse PRETTY_NAME from os-release
  {
    QFile f("/etc/os-release");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&f);
      while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("PRETTY_NAME=")) {
          prettyName = line.mid(12).remove('"').trimmed();
          break;
        }
      }
    }
  }
  if (!prettyName.isEmpty())
    parts << "\x02\x03" "10OS:\x03\x02 " + prettyName;
  else
    parts << "\x02\x03" "10OS:\x03\x02 " + QSysInfo::prettyProductName();

  // Kernel
  parts << "\x02\x03" "10Kernel:\x03\x02 " + QSysInfo::kernelType() + " " +
               QSysInfo::kernelVersion();

  // CPU — use readAll() since /proc files report size 0, breaking
  // QTextStream::atEnd()
  {
    QFile f("/proc/cpuinfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      const QStringList lines = QString::fromUtf8(f.readAll()).split('\n');
      int cores = 0;
      QString model;
      for (const QString &line : lines) {
        if (line.startsWith("model name") && model.isEmpty())
          model = line.section(':', 1).trimmed();
        if (line.startsWith("processor"))
          cores++;
      }
      if (!model.isEmpty())
        parts << "\x02\x03" "10CPU:\x03\x02 " + model + " (" + QString::number(cores) + " threads)";
    }
  }

  // RAM — use readAll() since /proc files report size 0, breaking
  // QTextStream::atEnd()
  {
    QFile f("/proc/meminfo");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      const QStringList lines = QString::fromUtf8(f.readAll()).split('\n');
      qint64 totalKb = 0, availKb = 0;
      for (const QString &line : lines) {
        if (line.startsWith("MemTotal:"))
          totalKb =
              line.split(QRegularExpression("\\s+")).value(1).toLongLong();
        else if (line.startsWith("MemAvailable:"))
          availKb =
              line.split(QRegularExpression("\\s+")).value(1).toLongLong();
      }
      if (totalKb > 0) {
        qint64 usedMb = (totalKb - availKb) / 1024;
        qint64 totalMb = totalKb / 1024;
        parts << "\x02\x03" "10RAM:\x03\x02 " + QString::number(usedMb) + "/" +
                     QString::number(totalMb) + " MB";
      }
    }
  }

  // Uptime
  {
    QString uptimeStr = readFileFirstLine("/proc/uptime");
    if (!uptimeStr.isEmpty()) {
      double secs = uptimeStr.section(' ', 0, 0).toDouble();
      int days = (int)(secs / 86400);
      int hours = ((int)secs % 86400) / 3600;
      int mins = ((int)secs % 3600) / 60;
      QString up;
      if (days > 0)
        up += QString::number(days) + "d ";
      up += QString::number(hours) + "h " + QString::number(mins) + "m";
      parts << "\x02\x03" "10Uptime:\x03\x02 " + up;
    }
  }

  // GPU (try lspci)
  {
    QString gpu = runProcess("lspci", {});
    if (!gpu.isEmpty()) {
      for (const QString &line : gpu.split('\n')) {
        if (line.contains("VGA") || line.contains("3D controller")) {
          QString name = line.section(':', 2).trimmed();
          if (!name.isEmpty()) {
            parts << "\x02\x03" "10GPU:\x03\x02 " + name;
            break;
          }
        }
      }
    }
  }

  // Shell
  {
    QString shell = qEnvironmentVariable("SHELL");
    if (!shell.isEmpty())
      parts << "\x02\x03" "10Shell:\x03\x02 " + shell.section('/', -1);
  }

  // Desktop
  {
    QString de = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (de.isEmpty())
      de = qEnvironmentVariable("DESKTOP_SESSION");
    if (!de.isEmpty())
      parts << "\x02\x03" "10DE:\x03\x02 " + de;
  }

  // Qt version
  parts << "\x02\x03" "10Qt:\x03\x02 " + QString::fromLatin1(qVersion());

  // NUchat version
  parts << "\x02\x03" "10NUchat\x03\x02 " NUCHAT_VERSION;

  return "\x02\x03" "06SysInfo:\x03\x02 " + parts.join(" | ");
}

IrcConnection *
IRCConnectionManager::connectionForServer(const QString &name) const {
  for (auto it = m_connToName.begin(); it != m_connToName.end(); ++it) {
    if (it.value() == name)
      return it.key();
  }
  return nullptr;
}

// ═══════════════════════════════════════════════════════════════════
//  Ignore List
// ═══════════════════════════════════════════════════════════════════

void IRCConnectionManager::addIgnore(const QString &mask) {
  QString normalized = mask;
  // If it's just a nick (no ! or @), convert to nick!*@*
  if (!normalized.contains('!') && !normalized.contains('@'))
    normalized = normalized + "!*@*";
  if (!m_ignoreList.contains(normalized, Qt::CaseInsensitive)) {
    m_ignoreList.append(normalized);
    if (m_settings) {
      m_settings->setValue("ignore/list", m_ignoreList);
      m_settings->sync();
    }
    emit ignoreListChanged();
  }
}

void IRCConnectionManager::removeIgnore(const QString &mask) {
  QString normalized = mask;
  if (!normalized.contains('!') && !normalized.contains('@'))
    normalized = normalized + "!*@*";
  // Case-insensitive removal
  for (int i = m_ignoreList.size() - 1; i >= 0; --i) {
    if (m_ignoreList[i].compare(normalized, Qt::CaseInsensitive) == 0) {
      m_ignoreList.removeAt(i);
    }
  }
  if (m_settings) {
    m_settings->setValue("ignore/list", m_ignoreList);
    m_settings->sync();
  }
  emit ignoreListChanged();
}

QStringList IRCConnectionManager::ignoreList() const { return m_ignoreList; }

void IRCConnectionManager::clearIgnoreList() {
  m_ignoreList.clear();
  if (m_settings) {
    m_settings->setValue("ignore/list", m_ignoreList);
    m_settings->sync();
  }
  emit ignoreListChanged();
}

bool IRCConnectionManager::isIgnored(const QString &nickOrMask) const {
  if (m_ignoreList.isEmpty())
    return false;

  for (const QString &pattern : m_ignoreList) {
    // Convert the ignore mask pattern to a regex: * → .*, ? → .
    QString regex = QRegularExpression::escape(pattern);
    regex.replace("\\*", ".*").replace("\\?", ".");
    QRegularExpression re("^" + regex + "$",
                          QRegularExpression::CaseInsensitiveOption);
    if (re.match(nickOrMask).hasMatch())
      return true;
    // Also try matching just the nick part if the input is a full prefix
    if (nickOrMask.contains('!')) {
      QString nick = nickOrMask.section('!', 0, 0);
      // Check if the pattern is a simple nick!*@* pattern
      QString patternNick = pattern.section('!', 0, 0);
      if (!patternNick.contains('*') && !patternNick.contains('?')) {
        if (nick.compare(patternNick, Qt::CaseInsensitive) == 0)
          return true;
      }
    }
  }
  return false;
}

// ═══════════════════════════════════════════════════════════════════
//  Auto-Reconnect
// ═══════════════════════════════════════════════════════════════════

void IRCConnectionManager::attemptReconnect(const QString &host) {
  if (!m_settings)
    return;

  bool autoReconnect = m_settings->getBool("conn/autoReconnect", true);
  if (!autoReconnect)
    return;

  if (!m_reconnectInfo.contains(host))
    return;

  ReconnectInfo &ri = m_reconnectInfo[host];
  int maxAttempts = m_settings->getInt("conn/maxReconnectAttempts", 10);
  const int baseDelay = m_settings->getInt("conn/reconnectDelay", 10);

  if (maxAttempts > 0 && ri.attempts >= maxAttempts) {
    QString msg = "Auto-reconnect: max attempts (" +
                  QString::number(maxAttempts) + ") reached for " + host;
    appendToChannel(host, host, "system", msg);
    if (m_msgModel && ircSame(m_activeServer, host))
      m_msgModel->addMessage("system", msg);
    return;
  }

  ri.attempts++;
  const int delayMs = reconnectDelayMs(baseDelay, ri.attempts);
  const int delay = delayMs / 1000;
  QString msg = "Auto-reconnect: attempting to reconnect to " + ri.host + " in " +
                QString::number(delay) + "s (attempt " +
                QString::number(ri.attempts) + "/" +
                (maxAttempts > 0 ? QString::number(maxAttempts) : "∞") + ")";
  appendToChannel(host, host, "system", msg);
  if (m_msgModel && ircSame(m_activeServer, host))
    m_msgModel->addMessage("system", msg);

  // Clean up old connection
  if (IrcConnection *oldConn = connectionForServer(host))
    retireConnection(oldConn);

  // Set up a timer
  if (ri.timer) {
    ri.timer->stop();
    ri.timer->deleteLater();
  }
  ri.timer = new QTimer(this);
  ri.timer->setSingleShot(true);
  connect(ri.timer, &QTimer::timeout, this, [this, host]() {
    if (!m_reconnectInfo.contains(host))
      return;
    ReconnectInfo &info = m_reconnectInfo[host];
    info.timer->deleteLater();
    info.timer = nullptr;

    // Create new connection with stored params
    auto *conn = new IrcConnection(this);
    conn->setNickname(info.nick);
    conn->setUser(info.user, info.realname);
    if (!info.password.isEmpty())
      conn->setPassword(info.password);
    if (!info.saslMethod.isEmpty() && info.saslMethod != "None")
      conn->setSaslAuth(info.saslMethod, info.saslUser, info.saslPass);
    if (!info.nickServPass.isEmpty()) {
      conn->setNickServCmd(info.nickServCmd);
      conn->setNickServPass(info.nickServPass);
    }

    m_connections.append(conn);
    m_connToName[conn] = host;

    wireConnection(conn);
    emit clientAdded(conn);

    // Do NOT touch m_activeServer/m_activeChannel here: a background
    // reconnect must not yank the user's view to the status buffer.
    QString reconnMsg =
        "Reconnecting to " + info.host + ":" + QString::number(info.port) + "...";
    appendToChannel(host, host, "system", reconnMsg);
    if (m_msgModel && ircSame(m_activeServer, host))
      m_msgModel->addMessage("system", reconnMsg);

    const QString proxyErr = applyProxySettings(conn);
    if (!proxyErr.isEmpty()) {
      appendToChannel(host, host, "system", proxyErr);
      if (m_msgModel && ircSame(m_activeServer, host))
        m_msgModel->addMessage("system", proxyErr);
      qWarning() << "[Manager]" << proxyErr;
      // Keep the retry chain alive instead of leaving a dead connection.
      attemptReconnect(host);
      return;
    }
    applyTlsPreferences(conn);
    conn->connectToServer(info.host, static_cast<quint16>(info.port), info.ssl);
  });
  ri.timer->start(delayMs);
}

// ═══════════════════════════════════════════════════════════════════
//  Perform-on-Connect
// ═══════════════════════════════════════════════════════════════════

void IRCConnectionManager::executePerformCommands(const QString &server) {
  if (!m_settings)
    return;

  QString performText =
      m_settings->getString("adv/performCommands", "").trimmed();
  if (performText.isEmpty())
    return;

  IrcConnection *conn = connectionForServer(server);
  if (!conn)
    return;

  QStringList commands = performText.split('\n', Qt::SkipEmptyParts);
  for (QString cmd : commands) {
    cmd = cmd.trimmed();
    if (cmd.isEmpty())
      continue;

    // Replace variables: %n = nick, %s = server
    cmd.replace("%n", conn->nickname());
    cmd.replace("%s", server);

    if (cmd.startsWith('/')) {
      // Execute as a command through the command dispatcher
      // Temporarily set active server so sendMessage routes correctly
      QString prevServer = m_activeServer;
      QString prevChannel = m_activeChannel;
      m_activeServer = server;
      m_activeChannel = server;
      sendMessage(server, cmd);
      m_activeServer = prevServer;
      m_activeChannel = prevChannel;
    } else {
      // Send raw
      conn->sendRaw(cmd);
    }
  }

  qDebug() << "[Manager] Executed" << commands.size() << "perform commands for"
           << server;
}

// ═══════════════════════════════════════════════════════════════════
//  URL Grabber
// ═══════════════════════════════════════════════════════════════════

void IRCConnectionManager::extractUrls(const QString &text, const QString &nick,
                                       const QString &channel) {
  // Check if URL grabbing is enabled
  if (m_settings) {
    bool grab = true;
    if (m_settings->contains(QStringLiteral("url/autoGrab")))
      grab = m_settings->getBool(QStringLiteral("url/autoGrab"), true);
    else if (m_settings->contains(QStringLiteral("url/grabber")))
      grab = m_settings->getBool(QStringLiteral("url/grabber"), true);
    if (!grab)
      return;
  }

  static QRegularExpression urlRe(R"((https?://[^\s<>"'\)\]]+))",
                                  QRegularExpression::CaseInsensitiveOption);

  auto it = urlRe.globalMatch(text);
  while (it.hasNext()) {
    auto match = it.next();
    GrabbedUrl entry;
    entry.url = match.captured(1);
    entry.nick = nick;
    entry.channel = channel;
    entry.timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_grabbedUrls.append(entry);
    emit urlGrabbed(entry.url, nick, channel);
  }
  // Evict oldest entries when over the cap
  if (m_grabbedUrls.size() > kMaxGrabbedUrls) {
    int excess = m_grabbedUrls.size() - kMaxGrabbedUrls;
    m_grabbedUrls.remove(0, excess);
  }
}

QVariantList IRCConnectionManager::grabbedUrls() const {
  QVariantList result;
  for (const auto &u : m_grabbedUrls) {
    QVariantMap m;
    m["url"] = u.url;
    m["nick"] = u.nick;
    m["channel"] = u.channel;
    m["timestamp"] = u.timestamp;
    result.append(m);
  }
  return result;
}

void IRCConnectionManager::clearGrabbedUrls() { m_grabbedUrls.clear(); }

// ═══════════════════════════════════════════════════════════════════
//  Away Log
// ═══════════════════════════════════════════════════════════════════

QVariantList IRCConnectionManager::awayLog() const {
  QVariantList result;
  const auto it = m_awayByServer.constFind(m_activeServer);
  if (it == m_awayByServer.constEnd())
    return result;
  for (const auto &e : it->log) {
    QVariantMap m;
    m["timestamp"] = e.timestamp;
    m["nick"] = e.nick;
    m["channel"] = e.channel;
    m["message"] = e.message;
    result.append(m);
  }
  return result;
}

void IRCConnectionManager::clearAwayLog() {
  auto it = m_awayByServer.find(m_activeServer);
  if (it == m_awayByServer.end())
    return;
  it->log.clear();
  emit awayLogUpdated();
}

bool IRCConnectionManager::isAway() const {
  const auto it = m_awayByServer.constFind(m_activeServer);
  return it != m_awayByServer.constEnd() && it->away;
}

void IRCConnectionManager::setServerAway(const QString &server, bool away,
                                         bool clearLog) {
  if (server.isEmpty())
    return;
  AwayState &st = m_awayByServer[server];
  st.away = away;
  if (clearLog)
    st.log.clear();
  if (ircSame(m_activeServer, server))
    emit awayStateChanged(away);
}

bool IRCConnectionManager::isViewing(const QString &server,
                                     const QString &channel) const {
  return ircSame(m_activeServer, server) &&
         sameIdentity(server, m_activeChannel, channel);
}

bool IRCConnectionManager::sameIdentity(const QString &server, const QString &a,
                                        const QString &b) const {
  if (auto *conn = connectionForServer(server))
    return conn->support().same(a, b);
  return ircEquals(a, b, IrcNetworkRegistry::factsFor(server).mapping);
}

bool IRCConnectionManager::isChannelName(const QString &server,
                                         const QString &name) const {
  if (auto *conn = connectionForServer(server))
    return conn->support().isChannel(name);
  const QString types = IrcNetworkRegistry::factsFor(server).chanTypes;
  return !name.isEmpty() && types.contains(name.at(0));
}

void IRCConnectionManager::publishSupport(const QString &server,
                                          const IrcSupport &support) {
  IrcNetworkFacts next;
  next.mapping = support.caseMapping();
  next.prefixSymbols = support.prefixSymbols();
  next.chanTypes = support.channelTypes();
  const IrcCaseMapping previous = IrcNetworkRegistry::factsFor(server).mapping;
  const bool mappingChanged = previous != next.mapping;

  auto take = [&](auto &map) {
    using Map = std::decay_t<decltype(map)>;
    Map extracted;
    if (!mappingChanged)
      return extracted;
    for (auto it = map.begin(); it != map.end();) {
      if (ircSame(it.key().server, server)) {
        extracted.insert(it.key(), it.value());
        it = map.erase(it);
      } else {
        ++it;
      }
    }
    return extracted;
  };
  auto hist = take(m_history);
  auto users = take(m_users);
  auto topics = take(m_topics);
  auto modes = take(m_modes);

  IrcNetworkRegistry::set(server, next);

  auto put = [&](auto &map, auto &extracted) {
    for (auto it = extracted.begin(); it != extracted.end(); ++it)
      map.insert(it.key(), it.value());
  };
  put(m_history, hist);
  put(m_users, users);
  put(m_topics, topics);
  put(m_modes, modes);
}

void IRCConnectionManager::applyTlsPreferences(IrcConnection *conn) {
  if (!conn || !m_settings)
    return;
  conn->setAllowSelfSignedCerts(
      m_settings->getBool(QStringLiteral("conn/allowSelfSignedCerts"), false));
  conn->setClientCertificateFiles(
      expandHomePath(m_settings->getString(QStringLiteral("auth/certFile"))),
      expandHomePath(m_settings->getString(QStringLiteral("auth/keyFile"))));
}

QString IRCConnectionManager::messageTimestamp(const QString &server) const {
  if (auto *conn = connectionForServer(server)) {
    const QDateTime when = conn->serverTime();
    if (when.isValid())
      return when.toLocalTime().toString(Qt::ISODate);
  }
  return QDateTime::currentDateTime().toString(Qt::ISODate);
}

void IRCConnectionManager::showOnOriginServer(const QString &server,
                                              const QString &type,
                                              const QString &text) {
  if (server.isEmpty())
    return;
  // If the user is looking at this server, keep the reply in that buffer.
  // Otherwise file it on the server tab so it cannot land in another
  // network's active channel.
  const QString dest =
      (ircSame(m_activeServer, server) && !m_activeChannel.isEmpty())
          ? m_activeChannel
          : server;
  appendToChannel(server, dest, type, text);
  if (m_msgModel && isViewing(server, dest))
    m_msgModel->addMessage(type, text);
}

void IRCConnectionManager::retireConnection(IrcConnection *conn) {
  if (!conn)
    return;
  // Drop slots before the object leaves the name map, so a late numeric
  // cannot be logged as server "unknown".
  QObject::disconnect(conn, nullptr, this, nullptr);
  m_connections.removeOne(conn);
  m_connToName.remove(conn);
  m_lagState.remove(conn);
  conn->deleteLater();
}

void IRCConnectionManager::trimHistory(QVector<StoredMessage> &hist) {
  int maxHistory = kMaxHistoryPerChannel;
  if (m_settings) {
    maxHistory = m_settings->getInt("ui/maxScrollback", kMaxHistoryPerChannel);
    if (maxHistory < 100)
      maxHistory = 100;
  }
  if (hist.size() > maxHistory) {
    const int excess = hist.size() - maxHistory;
    hist.remove(0, excess);
  }
}

void IRCConnectionManager::rememberEmbed(const QString &server,
                                         const QString &channel,
                                         const QString &html) {
  if (server.isEmpty() || channel.isEmpty() || html.isEmpty())
    return;
  ensureScrollbackLoaded(server, channel);
  ChannelKey key{server, channel};
  StoredMessage msg;
  msg.type = QStringLiteral("embed");
  msg.text = html;
  msg.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
  auto &hist = m_history[key];
  hist.append(msg);
  trimHistory(hist);
}

void IRCConnectionManager::refreshDccSettings() {
  if (!m_dccManager)
    return;
  m_dccManager->setDownloadDir(dccDownloadDirectory(m_settings));
  if (!m_settings)
    return;
  m_dccManager->setAutoAccept(
      m_settings->getBool(QStringLiteral("dcc/autoAccept"), false));
  m_dccManager->setMaxFileSize(dccMaxFileBytes(m_settings));
}

void IRCConnectionManager::openExternalUrl(const QString &url) {
  if (url.isEmpty())
    return;
  if (m_settings && !m_settings->getBool(QStringLiteral("url/clickable"), true))
    return;
  const QString cmd =
      m_settings ? m_settings->getString(QStringLiteral("url/browserCmd"))
                 : QString();
  if (cmd.isEmpty() || cmd == QLatin1String("xdg-open %s")) {
    QDesktopServices::openUrl(QUrl(url));
    return;
  }
  QStringList parts = QProcess::splitCommand(cmd);
  bool sawPlaceholder = false;
  for (QString &part : parts) {
    if (part.contains(QLatin1String("%s"))) {
      part.replace(QLatin1String("%s"), url);
      sawPlaceholder = true;
    }
  }
  if (!sawPlaceholder)
    parts.append(url);
  if (parts.isEmpty())
    return;
  const QString program = parts.takeFirst();
  QProcess::startDetached(program, parts);
}

// ═══════════════════════════════════════════════════════════════════
//  Proxy Support
// ═══════════════════════════════════════════════════════════════════

QString IRCConnectionManager::applyProxySettings(IrcConnection *conn) {
  if (!conn)
    return QStringLiteral("No connection");

  const NetSafety::ProxyResolution resolved = NetSafety::proxyFromSettings();
  if (resolved.result == NetSafety::ProxyResolution::Result::Rejected) {
    conn->setProxy(QNetworkProxy::NoProxy);
    return resolved.error;
  }
  if (resolved.result == NetSafety::ProxyResolution::Result::UseProxy) {
    const QNetworkProxy &p = resolved.proxy;
    conn->setProxy(p.type(), p.hostName(), static_cast<quint16>(p.port()), p.user(),
                   p.password());
  } else {
    conn->setProxy(QNetworkProxy::NoProxy);
  }
  return {};
}
