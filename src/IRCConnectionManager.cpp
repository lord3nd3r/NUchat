#include "IRCConnectionManager.h"
#include "IrcConnection.h"
#include "Logger.h"
#include "MessageModel.h"
#include "ServerChannelModel.h"
#include "Settings.h"
#include "Version.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QNetworkProxy>
#include <QProcess>
#include <QRegularExpression>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>
#include <algorithm>
#ifdef HAVE_PYTHON
#include "PythonScriptEngine.h"
#endif
#ifdef HAVE_LUA
#include "LuaScriptEngine.h"
#endif

IRCConnectionManager::IRCConnectionManager(QObject *parent) : QObject(parent) {}

IRCConnectionManager::~IRCConnectionManager() {
  for (auto *c : m_connections)
    c->disconnectFromServer("NUchat shutting down");
  qDeleteAll(m_connections);
}

void IRCConnectionManager::setMessageModel(MessageModel *model) {
  m_msgModel = model;
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
    const QString &nickServCmd, const QString &nickServPass) {
  qDebug() << "[Manager] connectToServer called:" << host << port << ssl << nick
           << "sasl:" << saslMethod << "nickserv:" << (!nickServPass.isEmpty());

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
  m_connToName[conn] = host;

  // Add to tree model
  if (m_treeModel) {
    m_treeModel->addServer(host);
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
  ri.attempts = 0;
  ri.timer = nullptr;
  m_reconnectInfo[host] = ri;
  m_userDisconnect = false;

  // Set as active server
  m_activeServer = host;
  m_activeChannel = host; // server tab

  // Record a system message
  if (m_msgModel) {
    m_msgModel->addMessage("system", "Connecting to " + host + ":" +
                                         QString::number(port) +
                                         (ssl ? " (SSL)" : "") + "...");
  }
  appendToChannel(host, host, "system",
                  "Connecting to " + host + ":" + QString::number(port) +
                      "...");

  applyProxySettings(conn);
  conn->connectToServer(host, static_cast<quint16>(port), ssl);
}

void IRCConnectionManager::disconnectFromServer(const QString &host) {
  m_userDisconnect = true;
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
  m_userDisconnect = true;
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
  if (channelName.startsWith('#') || channelName.startsWith('&')) {
    if (auto *conn = connectionForServer(serverName)) {
      conn->partChannel(channelName);
    }
  }
  if (m_treeModel) {
    m_treeModel->removeChannel(serverName, channelName);
  }
  emit channelParted(serverName, channelName);
}

void IRCConnectionManager::sendMessage(const QString &target,
                                       const QString &message) {
  if (auto *conn = activeConnection()) {
    // Handle /commands
    if (message.startsWith('/')) {
      QString cmd = message.section(' ', 0, 0).mid(1).toUpper();
      QString args = message.section(' ', 1);

      // ── ECHO — print locally without sending ──
      if (cmd == "ECHO" || cmd == "SAY" && false /* SAY handled below */) {
        if (cmd == "ECHO") {
          if (m_msgModel)
            m_msgModel->addMessage("system", args);
          return;
        }
      }

#ifdef HAVE_PYTHON
      // Let Python scripts intercept commands first
      if (PythonScriptEngine::instance()) {
        QStringList argParts = args.isEmpty() ? QStringList() : args.split(' ');
        if (PythonScriptEngine::instance()->handleCommand(cmd, argParts))
          return; // Script consumed the command
      }
#endif
#ifdef HAVE_LUA
      // Let Lua scripts intercept commands
      if (LuaScriptEngine::instance()) {
        QStringList argParts = args.isEmpty() ? QStringList() : args.split(' ');
        if (LuaScriptEngine::instance()->handleCommand(cmd, argParts))
          return; // Script consumed the command
      }
#endif

      if (cmd == "JOIN" || cmd == "J") {
        QString ch = args.section(' ', 0, 0);
        QString k = args.section(' ', 1, 1);
        conn->joinChannel(ch, k);
      } else if (cmd == "PART" || cmd == "LEAVE" || cmd == "P") {
        if (args.isEmpty())
          args = target;
        conn->partChannel(args);
      } else if (cmd == "NICK") {
        conn->changeNick(args.trimmed());
      } else if (cmd == "MSG" || cmd == "PRIVMSG" || cmd == "M" ||
                 cmd == "QUERY" || cmd == "Q") {
        QString tgt = args.section(' ', 0, 0);
        QString msg = args.section(' ', 1);
        // For private messages (not channels), open a query tab
        if (!tgt.startsWith('#') && !tgt.startsWith('&')) {
          openQuery(m_activeServer, tgt);
        }
        if (!msg.isEmpty()) {
          conn->sendMessage(tgt, msg);
          // Show our own message locally
          QString text = "<" + conn->nickname() + "> " + msg;
          appendToChannel(m_activeServer, tgt, "chat", text);
          if (m_msgModel && m_activeChannel == tgt)
            m_msgModel->addMessage("chat", text);
        }
      } else if (cmd == "ME") {
        conn->sendMessage(target, "\x01"
                                  "ACTION " +
                                      args + "\x01");
        // Show locally
        QString nick = conn->nickname();
        QString text = "* " + nick + " " + args;
        if (m_msgModel && m_activeChannel == target)
          m_msgModel->addMessage("action", text);
        appendToChannel(m_activeServer, target, "action", text);
      } else if (cmd == "QUIT" || cmd == "DISCONNECT" || cmd == "BYE") {
        m_userDisconnect = true;
        conn->disconnectFromServer(args.isEmpty() ? "NUchat" : args);
      } else if (cmd == "AWAY") {
        if (args.isEmpty()) {
          conn->setBack();
          m_isAway = false;
          emit awayStateChanged(false);
        } else {
          conn->setAway(args);
          m_isAway = true;
          m_awayLog.clear(); // Clear previous away log
          emit awayStateChanged(true);
        }
      } else if (cmd == "BACK") {
        conn->setBack();
        m_isAway = false;
        emit awayStateChanged(false);
      } else if (cmd == "WHOIS" || cmd == "W" || cmd == "WI") {
        conn->whois(args.trimmed());
      } else if (cmd == "RAW" || cmd == "QUOTE") {
        conn->sendRaw(args);
      } else if (cmd == "TOPIC" || cmd == "T") {
        if (args.isEmpty()) {
          conn->sendRaw("TOPIC " + target);
        } else {
          conn->sendRaw("TOPIC " + target + " :" + args);
        }
      } else if (cmd == "MODE") {
        conn->sendRaw("MODE " + (args.isEmpty() ? target : args));
      } else if (cmd == "KICK") {
        QString who = args.section(' ', 0, 0);
        QString reason = args.section(' ', 1);
        conn->sendRaw("KICK " + target + " " + who +
                      (reason.isEmpty() ? "" : " :" + reason));
      } else if (cmd == "NOTICE") {
        QString tgt = args.section(' ', 0, 0);
        QString msg = args.section(' ', 1);
        conn->sendNotice(tgt, msg);
      }
      // ── Service aliases (NickServ, ChanServ, OperServ, HostServ, MemoServ,
      // BotServ, HelpServ) ──
      else if (cmd == "NS" || cmd == "NICKSERV") {
        conn->sendMessage("NickServ", args);
      } else if (cmd == "CS" || cmd == "CHANSERV") {
        conn->sendMessage("ChanServ", args);
      } else if (cmd == "OS" || cmd == "OPERSERV") {
        conn->sendMessage("OperServ", args);
      } else if (cmd == "HS" || cmd == "HOSTSERV") {
        conn->sendMessage("HostServ", args);
      } else if (cmd == "MS" || cmd == "MEMOSERV") {
        conn->sendMessage("MemoServ", args);
      } else if (cmd == "BS" || cmd == "BOTSERV") {
        conn->sendMessage("BotServ", args);
      } else if (cmd == "HELPSERV") {
        conn->sendMessage("HelpServ", args);
      }
      // ── Channel user-mode shortcut aliases ──
      else if (cmd == "OP") {
        QString who = args.isEmpty() ? conn->nickname() : args.trimmed();
        conn->sendRaw("MODE " + target + " +o " + who);
      } else if (cmd == "DEOP") {
        QString who = args.isEmpty() ? conn->nickname() : args.trimmed();
        conn->sendRaw("MODE " + target + " -o " + who);
      } else if (cmd == "HALFOP" || cmd == "HOP") {
        QString who = args.isEmpty() ? conn->nickname() : args.trimmed();
        conn->sendRaw("MODE " + target + " +h " + who);
      } else if (cmd == "DEHALFOP" || cmd == "DEHOP") {
        QString who = args.isEmpty() ? conn->nickname() : args.trimmed();
        conn->sendRaw("MODE " + target + " -h " + who);
      } else if (cmd == "VOICE" || cmd == "V") {
        QString who = args.isEmpty() ? conn->nickname() : args.trimmed();
        conn->sendRaw("MODE " + target + " +v " + who);
      } else if (cmd == "DEVOICE") {
        QString who = args.isEmpty() ? conn->nickname() : args.trimmed();
        conn->sendRaw("MODE " + target + " -v " + who);
      }
      // ── Ban aliases ──
      else if (cmd == "BAN" || cmd == "B") {
        QString mask = args.trimmed();
        if (!mask.contains('!') && !mask.contains('@') && !mask.contains('*'))
          mask = mask + "!*@*";
        conn->sendRaw("MODE " + target + " +b " + mask);
      } else if (cmd == "UNBAN") {
        QString mask = args.trimmed();
        if (!mask.contains('!') && !mask.contains('@') && !mask.contains('*'))
          mask = mask + "!*@*";
        conn->sendRaw("MODE " + target + " -b " + mask);
      } else if (cmd == "KB" || cmd == "KICKBAN") {
        QString who = args.section(' ', 0, 0);
        QString reason = args.section(' ', 1);
        if (reason.isEmpty())
          reason = "Banned";
        QString mask = who;
        if (!mask.contains('!') && !mask.contains('@') && !mask.contains('*'))
          mask = who + "!*@*";
        conn->sendRaw("MODE " + target + " +b " + mask);
        conn->sendRaw("KICK " + target + " " + who + " :" + reason);
      }
      // ── Channel control aliases ──
      else if (cmd == "INVITE" || cmd == "INV") {
        QString who = args.trimmed();
        conn->sendRaw("INVITE " + who + " " + target);
      } else if (cmd == "CYCLE" || cmd == "REJOIN") {
        conn->partChannel(target, "Cycling");
        conn->joinChannel(target);
      } else if (cmd == "CLEAR") {
        if (m_msgModel)
          m_msgModel->clear();
      } else if (cmd == "CLOSE") {
        conn->partChannel(args.isEmpty() ? target : args.trimmed());
      }
      // ── CTCP ──
      else if (cmd == "CTCP") {
        QString tgt = args.section(' ', 0, 0);
        QString type = args.section(' ', 1, 1).toUpper();
        QString extra = args.section(' ', 2);
        if (type.isEmpty())
          type = "VERSION";
        QString ctcpMsg = "\x01" + type;
        if (!extra.isEmpty())
          ctcpMsg += " " + extra;
        ctcpMsg += "\x01";
        conn->sendMessage(tgt, ctcpMsg);
      }
      // ── Slap / action shortcuts ──
      else if (cmd == "SLAP") {
        QString who = args.trimmed();
        QString text = "\x01"
                       "ACTION slaps " +
                       who + " around a bit with a large trout\x01";
        conn->sendMessage(target, text);
        if (m_msgModel && m_activeChannel == target)
          m_msgModel->addMessage("action",
                                 "* " + conn->nickname() + " slaps " + who +
                                     " around a bit with a large trout");
        appendToChannel(m_activeServer, target, "action",
                        "* " + conn->nickname() + " slaps " + who +
                            " around a bit with a large trout");
      }
      // ── Network / Oper commands ──
      else if (cmd == "OPER") {
        conn->sendRaw("OPER " + args);
      } else if (cmd == "KILL") {
        QString who = args.section(' ', 0, 0);
        QString reason = args.section(' ', 1);
        conn->sendRaw("KILL " + who + (reason.isEmpty() ? "" : " :" + reason));
      } else if (cmd == "GLINE" || cmd == "KLINE" || cmd == "ZLINE" ||
                 cmd == "DLINE") {
        conn->sendRaw(cmd + " " + args);
      } else if (cmd == "WALLOPS") {
        conn->sendRaw("WALLOPS :" + args);
      } else if (cmd == "SQUIT") {
        conn->sendRaw("SQUIT " + args);
      } else if (cmd == "REHASH") {
        conn->sendRaw("REHASH");
      }
      // ── Info / lookup commands ──
      else if (cmd == "LINKS") {
        conn->sendRaw("LINKS" + (args.isEmpty() ? "" : " " + args));
      } else if (cmd == "LUSERS") {
        conn->sendRaw("LUSERS");
      } else if (cmd == "MOTD") {
        conn->sendRaw("MOTD" + (args.isEmpty() ? "" : " " + args));
      } else if (cmd == "ADMIN") {
        conn->sendRaw("ADMIN" + (args.isEmpty() ? "" : " " + args));
      } else if (cmd == "STATS") {
        conn->sendRaw("STATS " + args);
      } else if (cmd == "TRACE") {
        conn->sendRaw("TRACE " + args);
      } else if (cmd == "WHO") {
        conn->sendRaw("WHO " + (args.isEmpty() ? target : args));
      } else if (cmd == "NAMES") {
        conn->sendRaw("NAMES " + (args.isEmpty() ? target : args));
      } else if (cmd == "USERHOST") {
        conn->sendRaw("USERHOST " + args);
      } else if (cmd == "VERSION") {
        conn->sendRaw("VERSION" + (args.isEmpty() ? "" : " " + args));
      } else if (cmd == "TIME") {
        conn->sendRaw("TIME" + (args.isEmpty() ? "" : " " + args));
      } else if (cmd == "MAP") {
        conn->sendRaw("MAP");
      } else if (cmd == "LIST") {
        conn->sendRaw("LIST" + (args.isEmpty() ? "" : " " + args));
      } else if (cmd == "IGNORE") {
        if (args.trimmed().isEmpty()) {
          // Show current ignore list
          if (m_ignoreList.isEmpty()) {
            if (m_msgModel)
              m_msgModel->addMessage("system", "Ignore list is empty");
          } else {
            if (m_msgModel)
              m_msgModel->addMessage("system",
                                     "Ignore list (" +
                                         QString::number(m_ignoreList.size()) +
                                         " entries):");
            for (const auto &mask : m_ignoreList) {
              if (m_msgModel)
                m_msgModel->addMessage("system", "  " + mask);
            }
          }
        } else {
          addIgnore(args.trimmed());
          if (m_msgModel)
            m_msgModel->addMessage("system",
                                   "Added to ignore list: " + args.trimmed());
        }
      } else if (cmd == "UNIGNORE") {
        if (args.trimmed().isEmpty()) {
          if (m_msgModel)
            m_msgModel->addMessage("system",
                                   "Usage: /UNIGNORE <nick!user@host>");
        } else {
          removeIgnore(args.trimmed());
          if (m_msgModel)
            m_msgModel->addMessage("system", "Removed from ignore list: " +
                                                 args.trimmed());
        }
      }
      // ── System info ──
      else if (cmd == "SYSINFO") {
        QString info = gatherSysInfo();
        // Send to channel as ACTION so it looks like: * nick's system: ...
        QString actionText = "\x01"
                             "ACTION " +
                             info + "\x01";
        conn->sendRaw("PRIVMSG " + target + " :" + actionText);
        if (m_msgModel)
          m_msgModel->addMessage("action",
                                 "* " + conn->nickname() + " " + info);
        appendToChannel(m_activeServer, target, "action",
                        "* " + conn->nickname() + " " + info);
      } else {
        // Unknown /command — send as raw
        conn->sendRaw(cmd + " " + args);
      }
      return;
    }

    // Regular message
    conn->sendMessage(target, message);

    // Show our own message locally with status prefix
    QString nick = conn->nickname();
    QString displayNick = nick;
    ChannelKey key{m_activeServer, target};
    if (m_users.contains(key)) {
      for (const QString &u : m_users[key]) {
        QString bare = u;
        QString pfx;
        while (!bare.isEmpty() && QString("~&@%+").contains(bare[0])) {
          pfx += bare[0];
          bare = bare.mid(1);
        }
        if (bare.compare(nick, Qt::CaseInsensitive) == 0) {
          if (!pfx.isEmpty())
            displayNick = pfx[0] + nick;
          break;
        }
      }
    }
    QString text = "<" + displayNick + "> " + message;
    if (m_msgModel && m_activeChannel == target)
      m_msgModel->addMessage("chat", text);
    appendToChannel(m_activeServer, target, "chat", text);
  }
}

void IRCConnectionManager::sendRawCommand(const QString &raw) {
  if (auto *conn = activeConnection()) {
    conn->sendRaw(raw);
    emit rawLineReceived(">>", raw);
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

  // Clear unread/highlight for this channel
  clearUnread(serverName, channel);

  if (!m_msgModel)
    return;

  // Reload message history for this channel
  m_msgModel->clear();

  // Make sure we have loaded previous session's logs for this tab
  ensureScrollbackLoaded(serverName, channel);

  ChannelKey key{serverName, channel};
  if (m_history.contains(key) && !m_history[key].isEmpty()) {
    const auto &msgs = m_history[key];
    for (const auto &m : msgs)
      m_msgModel->addMessage(m.type, m.text);
  } else {
    m_msgModel->addMessage("system", "Now talking in " + channel);
  }

  emit currentNickChanged(currentNick());
  emit channelTopicChanged(channelTopic());
  emit channelUsersChanged(channelUsers());
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
  return MessageModel::ircToHtml(m_topics.value(key));
}

static int prefixRank(const QString &nick) {
  if (nick.isEmpty())
    return 99;
  QChar c = nick.at(0);
  if (c == '~')
    return 0; // owner
  if (c == '&')
    return 1; // admin / protected
  if (c == '@')
    return 2; // op
  if (c == '%')
    return 3; // halfop
  if (c == '+')
    return 4; // voice
  return 5;   // regular
}

QStringList IRCConnectionManager::channelUsers() const {
  ChannelKey key{m_activeServer, m_activeChannel};
  QStringList users = m_users.value(key);
  std::sort(users.begin(), users.end(), [](const QString &a, const QString &b) {
    int ra = prefixRank(a);
    int rb = prefixRank(b);
    if (ra != rb)
      return ra < rb;
    // Same rank: sort alphabetically (case-insensitive), stripping prefix
    QString na = (ra < 5) ? a.mid(1) : a;
    QString nb = (rb < 5) ? b.mid(1) : b;
    return na.compare(nb, Qt::CaseInsensitive) < 0;
  });
  return users;
}

// ── Private ──

void IRCConnectionManager::wireConnection(IrcConnection *conn) {
  QString host = m_connToName[conn];

  // Forward raw lines for Raw Log
  connect(conn, &IrcConnection::rawLineReceived, this,
          [this](const QString &line) { emit rawLineReceived("<<", line); });

  // Registered (001)
  connect(conn, &IrcConnection::registered, this, [this, conn]() {
    QString srv = serverNameFor(conn);
    QString msg = "Connected to " + srv + " as " + conn->nickname();
    appendToChannel(srv, srv, "system", msg);
    if (m_msgModel && m_activeServer == srv) {
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
  });

  // Disconnected
  connect(conn, &IrcConnection::disconnectedFromServer, this, [this, conn]() {
    QString srv = serverNameFor(conn);
    QString msg = "Disconnected from " + srv;
    appendToChannel(srv, srv, "system", msg);
    if (m_msgModel && m_activeServer == srv) {
      m_msgModel->addMessage("system", msg);
    }

    // ── Auto-reconnect ──
    if (!m_userDisconnect) {
      attemptReconnect(srv);
    }
    m_userDisconnect = false;
  });

  // Error
  connect(conn, &IrcConnection::errorOccurred, this,
          [this, conn](const QString &err) {
            QString srv = serverNameFor(conn);
            appendToChannel(srv, srv, "error", err);
            if (m_msgModel && m_activeServer == srv)
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
            // If target is our nick, it's a PM — use sender's nick as channel
            if (target == conn->nickname()) {
              channel = nick;
              // Auto-create query tab for incoming PMs
              if (m_treeModel && !m_treeModel->hasChannel(srv, nick)) {
                m_treeModel->addChannel(srv, nick);
                emit channelAdded(srv, nick);
              }
            }

            // Look up user's channel prefix (@, +, etc.)
            QString displayNick = nick;
            ChannelKey key{srv, channel};
            if (m_users.contains(key)) {
              for (const QString &u : m_users[key]) {
                QString bare = u;
                QString pfx;
                while (!bare.isEmpty() && QString("~&@%+").contains(bare[0])) {
                  pfx += bare[0];
                  bare = bare.mid(1);
                }
                if (bare.compare(nick, Qt::CaseInsensitive) == 0) {
                  if (!pfx.isEmpty())
                    displayNick = pfx[0] + nick;
                  break;
                }
              }
            }

            QString text = "<" + displayNick + "> " + message;
            appendToChannel(srv, channel, "chat", text);
            if (m_msgModel && m_activeServer == srv &&
                m_activeChannel == channel) {
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
            if (target.startsWith('#') || target.startsWith('&')) {
              dest = target;
            } else if (m_treeModel && m_treeModel->hasChannel(srv, nick)) {
              // Route to existing query window for this user
              dest = nick;
            } else {
              dest = srv;
            }
            appendToChannel(srv, dest, "system", text);
            if (m_msgModel && m_activeServer == srv &&
                m_activeChannel == dest) {
              m_msgModel->addMessage("system", text);
            }
          });

  // JOIN
  connect(
      conn, &IrcConnection::joinReceived, this,
      [this, conn](const QString &prefix, const QString &channel) {
        QString srv = serverNameFor(conn);
        QString nick =
            prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        if (nick == conn->nickname()) {
          // We joined — add channel to tree
          if (m_treeModel) {
            m_treeModel->addChannel(srv, channel);
          }
          emit channelJoined(srv, channel);

          // Load scrollback from log file before showing "Now talking in"
          ChannelKey key{srv, channel};
          if (m_logger && !m_history.contains(key)) {
            auto entries = m_logger->loadScrollback(srv, channel, 200);
            if (!entries.isEmpty()) {
              for (const auto &e : entries) {
                StoredMessage sm;
                sm.type = e.type;
                sm.text = e.text;
                sm.timestamp = e.timestamp;
                m_history[key].append(sm);
              }
            }
          }

          appendToChannel(srv, channel, "system", "Now talking in " + channel);
          // Auto-switch to the new channel
          m_activeChannel = channel;
          if (m_msgModel) {
            m_msgModel->clear();
            const auto &msgs = m_history[key];

            // Find where scrollback ends (the last "Now talking in" is the live
            // one we just appended)
            int scrollbackEnd =
                msgs.size() - 1; // last msg is our "Now talking in"
            bool hasScrollback = scrollbackEnd > 0;

            if (hasScrollback) {
              m_msgModel->addMessage(
                  "system", QString::fromUtf8(
                                "\u2500\u2500 Scrollback from %1 \u2500\u2500")
                                .arg(channel));
              for (int i = 0; i < scrollbackEnd; ++i)
                m_msgModel->addMessage(msgs[i].type, msgs[i].text);
              m_msgModel->addMessage(
                  "system", QString::fromUtf8(
                                "\u2500\u2500 End of scrollback \u2500\u2500"));
            }
            // Show the current "Now talking in" message
            m_msgModel->addMessage(msgs.last().type, msgs.last().text);
          }
          // Request channel modes so we can display them
          conn->sendRaw("MODE " + channel);
        } else {
          QString text = nick + " has joined " + channel;
          appendToChannel(srv, channel, "system", text);
          if (m_msgModel && m_activeServer == srv &&
              m_activeChannel == channel) {
            m_msgModel->addMessage("system", text);
          }
          // Add to user list
          ChannelKey key{srv, channel};
          if (m_users.contains(key) && !m_users[key].contains(nick)) {
            m_users[key].append(nick);
            if (m_activeServer == srv && m_activeChannel == channel)
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
        if (m_msgModel && m_activeServer == srv && m_activeChannel == channel) {
          m_msgModel->addMessage("system", text);
        }
        if (nick == conn->nickname()) {
          emit channelParted(srv, channel);
        }
        // Remove from user list
        ChannelKey key{srv, channel};
        if (m_users.contains(key)) {
          auto &users = m_users[key];
          users.erase(std::remove_if(
                          users.begin(), users.end(),
                          [&](const QString &u) {
                            QString bare = u;
                            while (!bare.isEmpty() &&
                                   QString("~&@%+").contains(bare[0]))
                              bare = bare.mid(1);
                            return bare.compare(nick, Qt::CaseInsensitive) == 0;
                          }),
                      users.end());
          if (m_activeServer == srv && m_activeChannel == channel)
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
          if (it.key().server != srv)
            continue;
          auto &users = it.value();
          int before = users.size();
          users.erase(std::remove_if(
                          users.begin(), users.end(),
                          [&](const QString &u) {
                            QString bare = u;
                            while (!bare.isEmpty() &&
                                   QString("~&@%+").contains(bare[0]))
                              bare = bare.mid(1);
                            return bare.compare(nick, Qt::CaseInsensitive) == 0;
                          }),
                      users.end());
          if (users.size() != before) {
            // User was in this channel — show quit message there
            appendToChannel(srv, it.key().channel, "system", text);
            if (m_activeServer == srv && m_activeChannel == it.key().channel)
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
        if (m_msgModel && m_activeServer == srv && m_activeChannel == channel) {
          m_msgModel->addMessage("system", text);
        }
        // Remove kicked user from list
        ChannelKey key{srv, channel};
        if (m_users.contains(key)) {
          auto &users = m_users[key];
          users.erase(
              std::remove_if(users.begin(), users.end(),
                             [&](const QString &u) {
                               QString bare = u;
                               while (!bare.isEmpty() &&
                                      QString("~&@%+").contains(bare[0]))
                                 bare = bare.mid(1);
                               return bare.compare(kicked,
                                                   Qt::CaseInsensitive) == 0;
                             }),
              users.end());
          if (m_activeServer == srv && m_activeChannel == channel)
            emit channelUsersChanged(channelUsers());
        }
      });

  // NICK change
  connect(conn, &IrcConnection::nickChanged, this,
          [this, conn](const QString &oldNick, const QString &newNick) {
            QString srv = serverNameFor(conn);
            QString text = oldNick + " is now known as " + newNick;
            if (oldNick == conn->nickname() || newNick == conn->nickname()) {
              emit currentNickChanged(conn->nickname());
            }
            // Update nick in all channel user lists on this server
            // and show the nick-change message only in channels where the user
            // is present
            bool emitUpdate = false;
            for (auto it = m_users.begin(); it != m_users.end(); ++it) {
              if (it.key().server != srv)
                continue;
              auto &users = it.value();
              for (int i = 0; i < users.size(); i++) {
                QString entry = users[i];
                QString prefix;
                QString bare = entry;
                while (!bare.isEmpty() && QString("~&@%+").contains(bare[0])) {
                  prefix += bare[0];
                  bare = bare.mid(1);
                }
                if (bare.compare(oldNick, Qt::CaseInsensitive) == 0) {
                  users[i] = prefix + newNick;
                  appendToChannel(srv, it.key().channel, "system", text);
                  if (m_activeServer == srv &&
                      m_activeChannel == it.key().channel) {
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
            if (m_msgModel && m_activeServer == srv &&
                m_activeChannel == channel) {
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
        QString dest = target.startsWith('#') ? target : srv;
        appendToChannel(srv, dest, "system", text);
        if (m_msgModel && m_activeServer == srv && m_activeChannel == dest) {
          m_msgModel->addMessage("system", text);
        }

        // Update user prefixes in the nick list for channel modes
        if (target.startsWith('#')) {
          ChannelKey key{srv, target};
          if (!m_users.contains(key))
            return;

          // Map mode chars to prefix symbols
          auto modeToPrefix = [](QChar m) -> QChar {
            if (m == 'q')
              return '~'; // owner
            if (m == 'a')
              return '&'; // admin
            if (m == 'o')
              return '@'; // op
            if (m == 'h')
              return '%'; // halfop
            if (m == 'v')
              return '+'; // voice
            return QChar();
          };

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

            QChar pfx = modeToPrefix(c);
            if (pfx.isNull() || paramIdx >= params.size()) {
              // Non-user mode that takes a param (b, k, l, etc.)
              if (QString("bkljeIfq").contains(c) && c != 'q')
                paramIdx++;
              else if (pfx.isNull()) { /* channel flag, no param */
              } else
                paramIdx++;
              continue;
            }

            QString affected = params[paramIdx++];

            // Find and update the user in the list
            for (int i = 0; i < users.size(); i++) {
              QString entry = users[i];
              QString bareNick = entry;
              QString currentPrefix;
              // Strip all existing prefixes
              while (!bareNick.isEmpty() &&
                     QString("~&@%+").contains(bareNick[0])) {
                currentPrefix += bareNick[0];
                bareNick = bareNick.mid(1);
              }
              if (bareNick.compare(affected, Qt::CaseInsensitive) != 0)
                continue;

              if (adding) {
                // Add prefix if not already present
                if (!currentPrefix.contains(pfx))
                  currentPrefix += pfx;
              } else {
                // Remove prefix
                currentPrefix.remove(pfx);
              }

              // Rebuild with highest-rank prefix only (IRC convention)
              QChar best;
              int bestRank = 99;
              for (QChar p : currentPrefix) {
                int r = 99;
                if (p == '~')
                  r = 0;
                else if (p == '&')
                  r = 1;
                else if (p == '@')
                  r = 2;
                else if (p == '%')
                  r = 3;
                else if (p == '+')
                  r = 4;
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
          if (m_activeServer == srv && m_activeChannel == target) {
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
            QString channel = target;
            if (target == conn->nickname()) {
              channel = nick; // PM
            }
            QString text;
            if (command == "ACTION") {
              text = "* " + nick + " " + args;
              appendToChannel(srv, channel, "action", text);
              if (m_msgModel && m_activeServer == srv &&
                  m_activeChannel == channel) {
                m_msgModel->addMessage("action", text);
              }
            } else {
              text = "CTCP " + command + " from " + nick +
                     (args.isEmpty() ? "" : ": " + args);
              appendToChannel(srv, srv, "system", text);
              if (m_msgModel && m_activeServer == srv && m_activeChannel == srv)
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
            if (m_activeServer == srv && m_activeChannel == channel) {
              emit channelUsersChanged(channelUsers());
            }
          });

  // Numeric replies (general — for MOTD etc.)
  connect(
      conn, &IrcConnection::numericReceived, this,
      [this, conn](int code, const QStringList &params,
                   const QString &trailing) {
        QString srv = serverNameFor(conn);

        // ── WHOIS replies (311–319, 330, 338, 378, 671) ──
        if (code == 311) {
          // RPL_WHOISUSER: <nick> <user> <host> * :<realname>
          QString nick = params.value(0);
          QString user = params.value(1);
          QString host = params.value(2);
          QString text =
              "[WHOIS] " + nick + " (" + user + "@" + host + ") : " + trailing;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 312) {
          // RPL_WHOISSERVER: <nick> <server> :<server info>
          QString nick = params.value(0);
          QString server = params.value(1);
          QString text = "[WHOIS] " + nick + " using server " + server + " (" +
                         trailing + ")";
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 313) {
          // RPL_WHOISOPERATOR
          QString nick = params.value(0);
          QString text = "[WHOIS] " + nick + " " + trailing;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 317) {
          // RPL_WHOISIDLE: <nick> <seconds> <signon> :seconds idle, signon time
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
          QString text = "[WHOIS] " + nick + " idle " + idle;
          if (!signonTime.isEmpty())
            text += ", signed on " + signonTime;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 318) {
          // RPL_ENDOFWHOIS
          QString nick = params.value(0);
          QString text = "[WHOIS] End of WHOIS for " + nick;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 319) {
          // RPL_WHOISCHANNELS: <nick> :<channels>
          QString nick = params.value(0);
          QString text = "[WHOIS] " + nick + " channels: " + trailing;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 330) {
          // RPL_WHOISACCOUNT: <nick> <account> :is logged in as
          QString nick = params.value(0);
          QString acct = params.value(1);
          QString text = "[WHOIS] " + nick + " " + trailing + " " + acct;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 338) {
          // RPL_WHOISACTUALLY: <nick> <ip> :actually using host
          QString nick = params.value(0);
          QString ip = params.value(1);
          QString text = "[WHOIS] " + nick + " actually using host " + ip;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 378) {
          // RPL_WHOISHOST: connecting from
          QString nick = params.value(0);
          QString text = "[WHOIS] " + nick + " " + trailing;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 671) {
          // RPL_WHOISSECURE
          QString nick = params.value(0);
          QString text = "[WHOIS] " + nick + " " + trailing;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        } else if (code == 324) {
          // RPL_CHANNELMODEIS: <channel> <modes> [<mode params>]
          QString channel = params.value(1);
          QString modes = params.value(2);
          QStringList modeParams = params.mid(3);
          QString text = "Channel modes for " + channel + ": " + modes;
          if (!modeParams.isEmpty())
            text += " " + modeParams.join(" ");
          appendToChannel(srv, channel, "system", text);
          if (m_msgModel && m_activeServer == srv && m_activeChannel == channel)
            m_msgModel->addMessage("system", text);
        } else if (code == 329) {
          // RPL_CREATIONTIME: <channel> <timestamp>
          QString channel = params.value(1);
          qint64 ts = params.value(2).toLongLong();
          QString timeStr = QDateTime::fromSecsSinceEpoch(ts).toString(
              "ddd MMM d hh:mm:ss yyyy");
          QString text = "Channel " + channel + " created on " + timeStr;
          appendToChannel(srv, channel, "system", text);
          if (m_msgModel && m_activeServer == srv && m_activeChannel == channel)
            m_msgModel->addMessage("system", text);
        } else if (code == 314) {
          // RPL_WHOWASUSER
          QString nick = params.value(1);
          QString user = params.value(2);
          QString host = params.value(3);
          QString text = "[WHOWAS] " + nick + " was " + user + "@" + host +
                         " : " + trailing;
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
        }

        // RPL_UNAWAY (305) and RPL_NOWAWAY (306) — track away state
        else if (code == 305) {
          // You are no longer marked as being away
          m_isAway = false;
          emit awayStateChanged(false);
          QString text = trailing.isEmpty()
                             ? "You are no longer marked as being away"
                             : trailing;
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
          // Show away log summary if there were messages
          if (!m_awayLog.isEmpty()) {
            if (m_msgModel)
              m_msgModel->addMessage(
                  "system", "You have " + QString::number(m_awayLog.size()) +
                                " message(s) in your away log. Use View > Away "
                                "Log to see them.");
          }
        } else if (code == 306) {
          // You have been marked as being away
          m_isAway = true;
          m_awayLog.clear();
          emit awayStateChanged(true);
          QString text = trailing.isEmpty()
                             ? "You have been marked as being away"
                             : trailing;
          appendToChannel(srv,
                          m_activeChannel.isEmpty() ? srv : m_activeChannel,
                          "system", text);
          if (m_msgModel)
            m_msgModel->addMessage("system", text);
        }

        // Show MOTD lines (372, 375, 376) and other informational numerics
        else if ((code >= 372 && code <= 376) || code == 2 || code == 3 ||
                 code == 4 || code == 5 || code == 251 || code == 252 ||
                 code == 253 || code == 254 || code == 255 || code == 265 ||
                 code == 266) {
          appendToChannel(srv, srv, "system", trailing);
          if (m_activeServer == srv &&
              (m_activeChannel.isEmpty() || m_activeChannel == srv)) {
            if (m_msgModel)
              m_msgModel->addMessage("system", trailing);
          }
        }
        // Error numerics — route to the relevant channel if mentioned in
        // params, else server tab
        if (code >= 400 && code < 600) {
          QString text = "[" + QString::number(code) + "] " + trailing;
          // Some error numerics mention a channel in params (e.g. 404 #channel
          // :Cannot send)
          QString dest = srv;
          for (int pi = 1; pi < params.size(); pi++) {
            if (params[pi].startsWith('#') || params[pi].startsWith('&')) {
              dest = params[pi];
              break;
            }
          }
          appendToChannel(srv, dest, "error", text);
          if (m_msgModel && m_activeServer == srv && m_activeChannel == dest)
            m_msgModel->addMessage("error", text);
        }
      });
}

void IRCConnectionManager::appendToChannel(const QString &server,
                                           const QString &channel,
                                           const QString &type,
                                           const QString &text) {
  // Make sure scrollback is populated into history before we start appending
  // new messages
  ensureScrollbackLoaded(server, channel);

  ChannelKey key{server, channel};
  StoredMessage msg;
  msg.type = type;
  msg.text = text;
  msg.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
  m_history[key].append(msg);

  // Log to file
  if (m_logger)
    m_logger->log(server, channel, type, text);

  // Track unread state for non-active channels
  bool isActive = (server == m_activeServer && channel == m_activeChannel);
  if (!isActive && (type == "chat" || type == "action")) {
    QString ukey = unreadKey(server, channel);
    bool changed = false;
    if (!m_unread.contains(ukey)) {
      m_unread.insert(ukey);
      changed = true;
    }
    // Check for nick highlight (mention of our nick in the message)
    if (auto *conn = connectionForServer(server)) {
      QString myNick = conn->nickname();
      if (!myNick.isEmpty() && text.contains(myNick, Qt::CaseInsensitive)) {
        if (!m_highlighted.contains(ukey)) {
          m_highlighted.insert(ukey);
          changed = true;
        }
      }
    }
    // Also mark all PMs (non-channel targets) as highlighted
    if (!channel.startsWith('#') && !channel.startsWith('&') &&
        channel != server) {
      if (!m_highlighted.contains(ukey)) {
        m_highlighted.insert(ukey);
        changed = true;
      }
    }
    if (changed)
      emit unreadStateChanged();
  }

  // ── URL Grabber — extract URLs from chat/action messages ──
  if (type == "chat" || type == "action") {
    // Extract the nick from the message text
    QString urlNick;
    if (type == "chat" && text.startsWith('<')) {
      urlNick = text.section('>', 0, 0).mid(1);
      // Strip mode prefix from nick
      while (!urlNick.isEmpty() && QString("~&@%+").contains(urlNick[0]))
        urlNick = urlNick.mid(1);
    } else if (type == "action" && text.startsWith("* ")) {
      urlNick = text.section(' ', 1, 1);
    }
    extractUrls(text, urlNick, channel);

    // ── Away log — capture messages while away ──
    if (m_isAway) {
      AwayLogEntry entry;
      entry.timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
      entry.nick = urlNick;
      entry.channel = channel;
      entry.message = text;
      m_awayLog.append(entry);
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
QString IRCConnectionManager::serverNameFor(IrcConnection *conn) const {
  return m_connToName.value(conn, "unknown");
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
    parts << "OS: " + prettyName;
  else
    parts << "OS: " + QSysInfo::prettyProductName();

  // Kernel
  parts << "Kernel: " + QSysInfo::kernelType() + " " +
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
        parts << "CPU: " + model + " (" + QString::number(cores) + " threads)";
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
        parts << "RAM: " + QString::number(usedMb) + "/" +
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
      parts << "Uptime: " + up;
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
            parts << "GPU: " + name;
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
      parts << "Shell: " + shell.section('/', -1);
  }

  // Desktop
  {
    QString de = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (de.isEmpty())
      de = qEnvironmentVariable("DESKTOP_SESSION");
    if (!de.isEmpty())
      parts << "DE: " + de;
  }

  // Qt version
  parts << "Qt: " + QString::fromLatin1(qVersion());

  // NUchat version
  parts << "NUchat " NUCHAT_VERSION;

  return "SysInfo: " + parts.join(" | ");
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

  bool autoReconnect = m_settings->value("conn/autoReconnect", true).toBool();
  if (!autoReconnect)
    return;

  if (!m_reconnectInfo.contains(host))
    return;

  ReconnectInfo &ri = m_reconnectInfo[host];
  int maxAttempts = m_settings->value("conn/maxReconnectAttempts", 10).toInt();
  int delay = m_settings->value("conn/reconnectDelay", 10).toInt();

  if (maxAttempts > 0 && ri.attempts >= maxAttempts) {
    QString msg = "Auto-reconnect: max attempts (" +
                  QString::number(maxAttempts) + ") reached for " + host;
    appendToChannel(host, host, "system", msg);
    if (m_msgModel && m_activeServer == host)
      m_msgModel->addMessage("system", msg);
    return;
  }

  ri.attempts++;
  QString msg = "Auto-reconnect: attempting to reconnect to " + host + " in " +
                QString::number(delay) + "s (attempt " +
                QString::number(ri.attempts) + "/" +
                (maxAttempts > 0 ? QString::number(maxAttempts) : "∞") + ")";
  appendToChannel(host, host, "system", msg);
  if (m_msgModel && m_activeServer == host)
    m_msgModel->addMessage("system", msg);

  // Clean up old connection
  IrcConnection *oldConn = connectionForServer(host);
  if (oldConn) {
    m_connections.removeOne(oldConn);
    m_connToName.remove(oldConn);
    oldConn->deleteLater();
  }

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

    m_activeServer = host;
    m_activeChannel = host;

    QString reconnMsg =
        "Reconnecting to " + host + ":" + QString::number(info.port) + "...";
    appendToChannel(host, host, "system", reconnMsg);
    if (m_msgModel)
      m_msgModel->addMessage("system", reconnMsg);

    applyProxySettings(conn);
    conn->connectToServer(host, static_cast<quint16>(info.port), info.ssl);
  });
  ri.timer->start(delay * 1000);
}

// ═══════════════════════════════════════════════════════════════════
//  Perform-on-Connect
// ═══════════════════════════════════════════════════════════════════

void IRCConnectionManager::executePerformCommands(const QString &server) {
  if (!m_settings)
    return;

  QString performText =
      m_settings->value("adv/performCommands", "").toString().trimmed();
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
  if (m_settings && !m_settings->value("url/autoGrab", true).toBool())
    return;

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
  for (const auto &e : m_awayLog) {
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
  m_awayLog.clear();
  emit awayLogUpdated();
}

// ═══════════════════════════════════════════════════════════════════
//  Proxy Support
// ═══════════════════════════════════════════════════════════════════

void IRCConnectionManager::applyProxySettings(IrcConnection *conn) {
  if (!m_settings || !conn)
    return;

  int typeIdx = m_settings->value("conn/proxyTypeIndex", 0).toInt();
  if (typeIdx <= 0) {
    conn->setProxy(QNetworkProxy::NoProxy);
    return;
  }

  QString host = m_settings->value("conn/proxyHost", "").toString().trimmed();
  int port = m_settings->value("conn/proxyPort", 1080).toInt();

  if (host.isEmpty())
    return;

  QNetworkProxy::ProxyType proxyType = QNetworkProxy::NoProxy;
  switch (typeIdx) {
  case 1:
    proxyType = QNetworkProxy::Socks5Proxy;
    break; // SOCKS4 not in Qt, fallback to SOCKS5
  case 2:
    proxyType = QNetworkProxy::Socks5Proxy;
    break; // SOCKS5
  case 3:
    proxyType = QNetworkProxy::HttpProxy;
    break; // HTTP CONNECT
  default:
    return;
  }

  // Authentication (user/pass from settings if proxy auth is enabled)
  bool useAuth = m_settings->value("conn/proxyAuth", false).toBool();
  QString user, pass;
  if (useAuth) {
    user = m_settings->value("conn/proxyUser", "").toString();
    pass = m_settings->value("conn/proxyPass", "").toString();
  }

  conn->setProxy(proxyType, host, static_cast<quint16>(port), user, pass);
}
