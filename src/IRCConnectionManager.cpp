#include "IRCConnectionManager.h"
#include "IrcConnection.h"
#include "MessageModel.h"
#include "ServerChannelModel.h"
#include <QDebug>
#include <QDateTime>
#include <QSysInfo>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>
#ifdef HAVE_PYTHON
#include "PythonScriptEngine.h"
#endif

IRCConnectionManager::IRCConnectionManager(QObject *parent)
    : QObject(parent)
{
}

IRCConnectionManager::~IRCConnectionManager()
{
    for (auto *c : m_connections)
        c->disconnectFromServer("NUchat shutting down");
    qDeleteAll(m_connections);
}

void IRCConnectionManager::setMessageModel(MessageModel *model)
{
    m_msgModel = model;
}

void IRCConnectionManager::setServerChannelModel(ServerChannelModel *model)
{
    m_treeModel = model;
}

void IRCConnectionManager::connectToServer(const QString &host, int port,
                                            bool ssl,
                                            const QString &nick,
                                            const QString &user,
                                            const QString &realname,
                                            const QString &password)
{
    qDebug() << "[Manager] connectToServer called:" << host << port << ssl << nick;

    auto *conn = new IrcConnection(this);
    conn->setNickname(nick);
    conn->setUser(user, realname);
    if (!password.isEmpty())
        conn->setPassword(password);

    m_connections.append(conn);
    m_connToName[conn] = host;

    // Add to tree model
    if (m_treeModel) {
        m_treeModel->addServer(host);
    }

    wireConnection(conn);
    emit clientAdded(conn);

    // Set as active server
    m_activeServer = host;
    m_activeChannel = host;  // server tab

    // Record a system message
    if (m_msgModel) {
        m_msgModel->addMessage("system", "Connecting to " + host + ":" + QString::number(port) +
                               (ssl ? " (SSL)" : "") + "...");
    }
    appendToChannel(host, host, "system", "Connecting to " + host + ":" + QString::number(port) + "...");

    conn->connectToServer(host, static_cast<quint16>(port), ssl);
}

void IRCConnectionManager::disconnectFromServer(const QString &host)
{
    if (auto *conn = connectionForServer(host)) {
        conn->disconnectFromServer();
    }
}

void IRCConnectionManager::disconnectAll()
{
    for (auto *c : m_connections)
        c->disconnectFromServer();
}

void IRCConnectionManager::joinChannel(const QString &channel, const QString &key)
{
    if (auto *conn = activeConnection()) {
        conn->joinChannel(channel, key);
    }
}

void IRCConnectionManager::partChannel(const QString &channel, const QString &reason)
{
    if (auto *conn = activeConnection()) {
        conn->partChannel(channel, reason);
    }
}

void IRCConnectionManager::sendMessage(const QString &target, const QString &message)
{
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
                    return;  // Script consumed the command
            }
#endif

            if (cmd == "JOIN" || cmd == "J") {
                QString ch = args.section(' ', 0, 0);
                QString k = args.section(' ', 1, 1);
                conn->joinChannel(ch, k);
            } else if (cmd == "PART" || cmd == "LEAVE" || cmd == "P") {
                if (args.isEmpty()) args = target;
                conn->partChannel(args);
            } else if (cmd == "NICK") {
                conn->changeNick(args.trimmed());
            } else if (cmd == "MSG" || cmd == "PRIVMSG" || cmd == "M" || cmd == "QUERY" || cmd == "Q") {
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
                conn->sendMessage(target, "\x01" "ACTION " + args + "\x01");
                // Show locally
                QString nick = conn->nickname();
                QString text = "* " + nick + " " + args;
                if (m_msgModel && m_activeChannel == target)
                    m_msgModel->addMessage("action", text);
                appendToChannel(m_activeServer, target, "action", text);
            } else if (cmd == "QUIT" || cmd == "DISCONNECT" || cmd == "BYE") {
                conn->disconnectFromServer(args.isEmpty() ? "NUchat" : args);
            } else if (cmd == "AWAY") {
                if (args.isEmpty()) conn->setBack();
                else conn->setAway(args);
            } else if (cmd == "BACK") {
                conn->setBack();
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
                conn->sendRaw("KICK " + target + " " + who + (reason.isEmpty() ? "" : " :" + reason));
            } else if (cmd == "NOTICE") {
                QString tgt = args.section(' ', 0, 0);
                QString msg = args.section(' ', 1);
                conn->sendNotice(tgt, msg);
            }
            // ── Service aliases (NickServ, ChanServ, OperServ, HostServ, MemoServ, BotServ, HelpServ) ──
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
                if (reason.isEmpty()) reason = "Banned";
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
                if (type.isEmpty()) type = "VERSION";
                QString ctcpMsg = "\x01" + type;
                if (!extra.isEmpty()) ctcpMsg += " " + extra;
                ctcpMsg += "\x01";
                conn->sendMessage(tgt, ctcpMsg);
            }
            // ── Slap / action shortcuts ──
            else if (cmd == "SLAP") {
                QString who = args.trimmed();
                QString text = "\x01" "ACTION slaps " + who + " around a bit with a large trout\x01";
                conn->sendMessage(target, text);
                if (m_msgModel && m_activeChannel == target)
                    m_msgModel->addMessage("action", "* " + conn->nickname() + " slaps " + who + " around a bit with a large trout");
                appendToChannel(m_activeServer, target, "action", "* " + conn->nickname() + " slaps " + who + " around a bit with a large trout");
            }
            // ── Network / Oper commands ──
            else if (cmd == "OPER") {
                conn->sendRaw("OPER " + args);
            } else if (cmd == "KILL") {
                QString who = args.section(' ', 0, 0);
                QString reason = args.section(' ', 1);
                conn->sendRaw("KILL " + who + (reason.isEmpty() ? "" : " :" + reason));
            } else if (cmd == "GLINE" || cmd == "KLINE" || cmd == "ZLINE" || cmd == "DLINE") {
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
                if (m_msgModel)
                    m_msgModel->addMessage("system", "Ignore list: not yet implemented");
            } else if (cmd == "UNIGNORE") {
                if (m_msgModel)
                    m_msgModel->addMessage("system", "Ignore list: not yet implemented");
            }
            // ── System info ──
            else if (cmd == "SYSINFO") {
                QString info = gatherSysInfo();
                // Send to channel as ACTION so it looks like: * nick's system: ...
                QString actionText = "\x01" "ACTION " + info + "\x01";
                conn->sendRaw("PRIVMSG " + target + " :" + actionText);
                if (m_msgModel)
                    m_msgModel->addMessage("action", "* " + conn->nickname() + " " + info);
                appendToChannel(m_activeServer, target, "action", "* " + conn->nickname() + " " + info);
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

void IRCConnectionManager::sendRawCommand(const QString &raw)
{
    if (auto *conn = activeConnection()) {
        conn->sendRaw(raw);
        emit rawLineReceived(">>", raw);
    }
}

void IRCConnectionManager::changeNick(const QString &newNick)
{
    if (auto *conn = activeConnection())
        conn->changeNick(newNick);
}

void IRCConnectionManager::setActiveServer(const QString &host)
{
    m_activeServer = host;
}

void IRCConnectionManager::setActiveChannel(const QString &channel)
{
    m_activeChannel = channel;
}

void IRCConnectionManager::switchToChannel(const QString &serverName, const QString &channel)
{
    m_activeServer = serverName;
    m_activeChannel = channel;

    if (!m_msgModel) return;

    // Reload message history for this channel
    m_msgModel->clear();
    ChannelKey key{serverName, channel};
    if (m_history.contains(key)) {
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

void IRCConnectionManager::openQuery(const QString &serverName, const QString &nick)
{
    if (nick.isEmpty() || serverName.isEmpty()) return;

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

IrcConnection *IRCConnectionManager::activeConnection() const
{
    return connectionForServer(m_activeServer);
}

QString IRCConnectionManager::currentNick() const
{
    if (auto *conn = activeConnection())
        return conn->nickname();
    return "NUchat_user";
}

QString IRCConnectionManager::channelTopic() const
{
    ChannelKey key{m_activeServer, m_activeChannel};
    return MessageModel::ircToHtml(m_topics.value(key));
}

static int prefixRank(const QString &nick)
{
    if (nick.isEmpty()) return 99;
    QChar c = nick.at(0);
    if (c == '~') return 0;  // owner
    if (c == '&') return 1;  // admin / protected
    if (c == '@') return 2;  // op
    if (c == '%') return 3;  // halfop
    if (c == '+') return 4;  // voice
    return 5;                // regular
}

QStringList IRCConnectionManager::channelUsers() const
{
    ChannelKey key{m_activeServer, m_activeChannel};
    QStringList users = m_users.value(key);
    std::sort(users.begin(), users.end(), [](const QString &a, const QString &b) {
        int ra = prefixRank(a);
        int rb = prefixRank(b);
        if (ra != rb) return ra < rb;
        // Same rank: sort alphabetically (case-insensitive), stripping prefix
        QString na = (ra < 5) ? a.mid(1) : a;
        QString nb = (rb < 5) ? b.mid(1) : b;
        return na.compare(nb, Qt::CaseInsensitive) < 0;
    });
    return users;
}

// ── Private ──

void IRCConnectionManager::wireConnection(IrcConnection *conn)
{
    QString host = m_connToName[conn];

    // Forward raw lines for Raw Log
    connect(conn, &IrcConnection::rawLineReceived, this, [this](const QString &line) {
        emit rawLineReceived("<<", line);
    });

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
    });

    // Disconnected
    connect(conn, &IrcConnection::disconnectedFromServer, this, [this, conn]() {
        QString srv = serverNameFor(conn);
        QString msg = "Disconnected from " + srv;
        appendToChannel(srv, srv, "system", msg);
        if (m_msgModel && m_activeServer == srv) {
            m_msgModel->addMessage("system", msg);
        }
    });

    // Error
    connect(conn, &IrcConnection::errorOccurred, this, [this, conn](const QString &err) {
        QString srv = serverNameFor(conn);
        appendToChannel(srv, srv, "error", err);
        if (m_msgModel && m_activeServer == srv)
            m_msgModel->addMessage("error", err);
    });

    // PRIVMSG
    connect(conn, &IrcConnection::privmsgReceived, this,
        [this, conn](const QString &prefix, const QString &target, const QString &message) {
        QString srv = serverNameFor(conn);
        QString nick = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString channel = target;
        // If target is our nick, it's a PM — use sender's nick as channel
        if (target == conn->nickname()) {
            channel = nick;
            // Auto-create query tab for incoming PMs
            if (m_treeModel && !m_treeModel->hasChannel(srv, nick)) {
                m_treeModel->addChannel(srv, nick);
                emit channelJoined(srv, nick);
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
        if (m_msgModel && m_activeServer == srv && m_activeChannel == channel) {
            m_msgModel->addMessage("chat", text);
        }
    });

    // NOTICE
    connect(conn, &IrcConnection::noticeReceived, this,
        [this, conn](const QString &prefix, const QString &target, const QString &message) {
        QString srv = serverNameFor(conn);
        QString nick = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = "-" + nick + "- " + message;
        // Route to the target channel if it's a channel, otherwise to the server tab
        QString dest;
        if (target.startsWith('#') || target.startsWith('&')) {
            dest = target;
        } else {
            dest = srv;
        }
        appendToChannel(srv, dest, "system", text);
        if (m_msgModel && m_activeServer == srv && m_activeChannel == dest) {
            m_msgModel->addMessage("system", text);
        }
    });

    // JOIN
    connect(conn, &IrcConnection::joinReceived, this,
        [this, conn](const QString &prefix, const QString &channel) {
        QString srv = serverNameFor(conn);
        QString nick = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        if (nick == conn->nickname()) {
            // We joined — add channel to tree
            if (m_treeModel) {
                m_treeModel->addChannel(srv, channel);
            }
            emit channelJoined(srv, channel);
            appendToChannel(srv, channel, "system", "Now talking in " + channel);
            // Auto-switch to the new channel
            m_activeChannel = channel;
            if (m_msgModel) {
                m_msgModel->clear();
                m_msgModel->addMessage("system", "Now talking in " + channel);
            }
        } else {
            QString text = nick + " has joined " + channel;
            appendToChannel(srv, channel, "system", text);
            if (m_msgModel && m_activeServer == srv && m_activeChannel == channel) {
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
    connect(conn, &IrcConnection::partReceived, this,
        [this, conn](const QString &prefix, const QString &channel, const QString &reason) {
        QString srv = serverNameFor(conn);
        QString nick = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = nick + " has left " + channel;
        if (!reason.isEmpty()) text += " (" + reason + ")";
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
            users.erase(std::remove_if(users.begin(), users.end(), [&](const QString &u) {
                QString bare = u;
                while (!bare.isEmpty() && QString("~&@%+").contains(bare[0])) bare = bare.mid(1);
                return bare.compare(nick, Qt::CaseInsensitive) == 0;
            }), users.end());
            if (m_activeServer == srv && m_activeChannel == channel)
                emit channelUsersChanged(channelUsers());
        }
    });

    // QUIT
    connect(conn, &IrcConnection::quitReceived, this,
        [this, conn](const QString &prefix, const QString &reason) {
        QString srv = serverNameFor(conn);
        QString nick = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = nick + " has quit";
        if (!reason.isEmpty()) text += " (" + reason + ")";
        // Remove from all channels on this server and show quit message
        bool emitUpdate = false;
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.key().server != srv) continue;
            auto &users = it.value();
            int before = users.size();
            users.erase(std::remove_if(users.begin(), users.end(), [&](const QString &u) {
                QString bare = u;
                while (!bare.isEmpty() && QString("~&@%+").contains(bare[0])) bare = bare.mid(1);
                return bare.compare(nick, Qt::CaseInsensitive) == 0;
            }), users.end());
            if (users.size() != before) {
                // User was in this channel — show quit message there
                appendToChannel(srv, it.key().channel, "system", text);
                if (m_activeServer == srv && m_activeChannel == it.key().channel)
                    emitUpdate = true;
            }
        }
        if (emitUpdate) {
            if (m_msgModel) m_msgModel->addMessage("system", text);
            emit channelUsersChanged(channelUsers());
        }
    });

    // KICK
    connect(conn, &IrcConnection::kickReceived, this,
        [this, conn](const QString &prefix, const QString &channel, const QString &kicked, const QString &reason) {
        QString srv = serverNameFor(conn);
        QString kicker = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = kicked + " was kicked by " + kicker;
        if (!reason.isEmpty()) text += " (" + reason + ")";
        appendToChannel(srv, channel, "system", text);
        if (m_msgModel && m_activeServer == srv && m_activeChannel == channel) {
            m_msgModel->addMessage("system", text);
        }
        // Remove kicked user from list
        ChannelKey key{srv, channel};
        if (m_users.contains(key)) {
            auto &users = m_users[key];
            users.erase(std::remove_if(users.begin(), users.end(), [&](const QString &u) {
                QString bare = u;
                while (!bare.isEmpty() && QString("~&@%+").contains(bare[0])) bare = bare.mid(1);
                return bare.compare(kicked, Qt::CaseInsensitive) == 0;
            }), users.end());
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
        // and show the nick-change message only in channels where the user is present
        bool emitUpdate = false;
        for (auto it = m_users.begin(); it != m_users.end(); ++it) {
            if (it.key().server != srv) continue;
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
                    if (m_activeServer == srv && m_activeChannel == it.key().channel) {
                        emitUpdate = true;
                        if (m_msgModel) m_msgModel->addMessage("system", text);
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
        if (m_msgModel && m_activeServer == srv && m_activeChannel == channel) {
            m_msgModel->addMessage("system", text);
            emit channelTopicChanged(topic);
        }
    });

    // MODE
    connect(conn, &IrcConnection::modeReceived, this,
        [this, conn](const QString &prefix, const QString &target, const QString &modeStr, const QStringList &params) {
        QString srv = serverNameFor(conn);
        QString nick = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        QString text = nick + " sets mode " + modeStr;
        if (!params.isEmpty()) text += " " + params.join(" ");
        text += " on " + target;
        QString dest = target.startsWith('#') ? target : srv;
        appendToChannel(srv, dest, "system", text);
        if (m_msgModel && m_activeServer == srv && m_activeChannel == dest) {
            m_msgModel->addMessage("system", text);
        }

        // Update user prefixes in the nick list for channel modes
        if (target.startsWith('#')) {
            ChannelKey key{srv, target};
            if (!m_users.contains(key)) return;

            // Map mode chars to prefix symbols
            auto modeToPrefix = [](QChar m) -> QChar {
                if (m == 'q') return '~';   // owner
                if (m == 'a') return '&';   // admin
                if (m == 'o') return '@';   // op
                if (m == 'h') return '%';   // halfop
                if (m == 'v') return '+';   // voice
                return QChar();
            };

            bool adding = true;
            int paramIdx = 0;
            QStringList &users = m_users[key];

            for (QChar c : modeStr) {
                if (c == '+') { adding = true; continue; }
                if (c == '-') { adding = false; continue; }

                QChar pfx = modeToPrefix(c);
                if (pfx.isNull() || paramIdx >= params.size()) {
                    // Non-user mode that takes a param (b, k, l, etc.)
                    if (QString("bkljeIfq").contains(c) && c != 'q') paramIdx++;
                    else if (pfx.isNull()) { /* channel flag, no param */ }
                    else paramIdx++;
                    continue;
                }

                QString affected = params[paramIdx++];

                // Find and update the user in the list
                for (int i = 0; i < users.size(); i++) {
                    QString entry = users[i];
                    QString bareNick = entry;
                    QString currentPrefix;
                    // Strip all existing prefixes
                    while (!bareNick.isEmpty() && QString("~&@%+").contains(bareNick[0])) {
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
                        if (p == '~') r = 0;
                        else if (p == '&') r = 1;
                        else if (p == '@') r = 2;
                        else if (p == '%') r = 3;
                        else if (p == '+') r = 4;
                        if (r < bestRank) { bestRank = r; best = p; }
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
        [this, conn](const QString &prefix, const QString &target, const QString &command, const QString &args) {
        QString srv = serverNameFor(conn);
        QString nick = prefix.contains('!') ? prefix.section('!', 0, 0) : prefix;
        // Determine the proper destination channel
        QString channel = target;
        if (target == conn->nickname()) {
            channel = nick;  // PM
        }
        QString text;
        if (command == "ACTION") {
            text = "* " + nick + " " + args;
            appendToChannel(srv, channel, "action", text);
            if (m_msgModel && m_activeServer == srv && m_activeChannel == channel) {
                m_msgModel->addMessage("action", text);
            }
        } else {
            text = "CTCP " + command + " from " + nick + (args.isEmpty() ? "" : ": " + args);
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
    connect(conn, &IrcConnection::numericReceived, this,
        [this, conn](int code, const QStringList &params, const QString &trailing) {
        QString srv = serverNameFor(conn);

        // ── WHOIS replies (311–319, 330, 338, 378, 671) ──
        if (code == 311) {
            // RPL_WHOISUSER: <nick> <user> <host> * :<realname>
            QString nick = params.value(1);
            QString user = params.value(2);
            QString host = params.value(3);
            QString text = "[WHOIS] " + nick + " (" + user + "@" + host + ") : " + trailing;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 312) {
            // RPL_WHOISSERVER: <nick> <server> :<server info>
            QString nick = params.value(1);
            QString server = params.value(2);
            QString text = "[WHOIS] " + nick + " using server " + server + " (" + trailing + ")";
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 313) {
            // RPL_WHOISOPERATOR
            QString nick = params.value(1);
            QString text = "[WHOIS] " + nick + " " + trailing;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 317) {
            // RPL_WHOISIDLE: <nick> <seconds> <signon> :seconds idle, signon time
            QString nick = params.value(1);
            int idleSecs = params.value(2).toInt();
            int d = idleSecs / 86400, h = (idleSecs % 86400) / 3600, m = (idleSecs % 3600) / 60, s = idleSecs % 60;
            QString idle;
            if (d > 0) idle += QString::number(d) + "d ";
            if (h > 0) idle += QString::number(h) + "h ";
            if (m > 0) idle += QString::number(m) + "m ";
            idle += QString::number(s) + "s";
            QString signonTime;
            if (params.size() > 3) {
                qint64 ts = params.value(3).toLongLong();
                signonTime = QDateTime::fromSecsSinceEpoch(ts).toString("yyyy-MM-dd hh:mm:ss");
            }
            QString text = "[WHOIS] " + nick + " idle " + idle;
            if (!signonTime.isEmpty()) text += ", signed on " + signonTime;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 318) {
            // RPL_ENDOFWHOIS
            QString nick = params.value(1);
            QString text = "[WHOIS] End of WHOIS for " + nick;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 319) {
            // RPL_WHOISCHANNELS: <nick> :<channels>
            QString nick = params.value(1);
            QString text = "[WHOIS] " + nick + " channels: " + trailing;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 330) {
            // RPL_WHOISACCOUNT: <nick> <account> :is logged in as
            QString nick = params.value(1);
            QString acct = params.value(2);
            QString text = "[WHOIS] " + nick + " " + trailing + " " + acct;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 338) {
            // RPL_WHOISACTUALLY: <nick> <ip> :actually using host
            QString nick = params.value(1);
            QString ip = params.value(2);
            QString text = "[WHOIS] " + nick + " actually using host " + ip;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 378) {
            // RPL_WHOISHOST: connecting from
            QString nick = params.value(1);
            QString text = "[WHOIS] " + nick + " " + trailing;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 671) {
            // RPL_WHOISSECURE
            QString nick = params.value(1);
            QString text = "[WHOIS] " + nick + " " + trailing;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        } else if (code == 314) {
            // RPL_WHOWASUSER
            QString nick = params.value(1);
            QString user = params.value(2);
            QString host = params.value(3);
            QString text = "[WHOWAS] " + nick + " was " + user + "@" + host + " : " + trailing;
            if (m_msgModel) m_msgModel->addMessage("system", text);
            appendToChannel(srv, m_activeChannel.isEmpty() ? srv : m_activeChannel, "system", text);
        }

        // Show MOTD lines (372, 375, 376) and other informational numerics
        else if ((code >= 372 && code <= 376) || code == 2 || code == 3 || code == 4 ||
            code == 5 || code == 251 || code == 252 || code == 253 || code == 254 ||
            code == 255 || code == 265 || code == 266) {
            appendToChannel(srv, srv, "system", trailing);
            if (m_activeServer == srv && (m_activeChannel.isEmpty() || m_activeChannel == srv)) {
                if (m_msgModel)
                    m_msgModel->addMessage("system", trailing);
            }
        }
        // Error numerics — route to the relevant channel if mentioned in params, else server tab
        if (code >= 400 && code < 600) {
            QString text = "[" + QString::number(code) + "] " + trailing;
            // Some error numerics mention a channel in params (e.g. 404 #channel :Cannot send)
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

void IRCConnectionManager::appendToChannel(const QString &server, const QString &channel,
                                            const QString &type, const QString &text)
{
    ChannelKey key{server, channel};
    StoredMessage msg;
    msg.type = type;
    msg.text = text;
    msg.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_history[key].append(msg);
}

QString IRCConnectionManager::serverNameFor(IrcConnection *conn) const
{
    return m_connToName.value(conn, "unknown");
}

// ── /SYSINFO helper ──
static QString readFileFirstLine(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QTextStream(&f).readLine().trimmed();
    }
    return {};
}

static QString runProcess(const QString &program, const QStringList &args, int timeout = 2000)
{
    QProcess proc;
    proc.start(program, args);
    if (proc.waitForFinished(timeout))
        return proc.readAllStandardOutput().trimmed();
    return {};
}

QString IRCConnectionManager::gatherSysInfo()
{
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
    parts << "Kernel: " + QSysInfo::kernelType() + " " + QSysInfo::kernelVersion();

    // CPU — use readAll() since /proc files report size 0, breaking QTextStream::atEnd()
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

    // RAM — use readAll() since /proc files report size 0, breaking QTextStream::atEnd()
    {
        QFile f("/proc/meminfo");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QStringList lines = QString::fromUtf8(f.readAll()).split('\n');
            qint64 totalKb = 0, availKb = 0;
            for (const QString &line : lines) {
                if (line.startsWith("MemTotal:"))
                    totalKb = line.split(QRegularExpression("\\s+")).value(1).toLongLong();
                else if (line.startsWith("MemAvailable:"))
                    availKb = line.split(QRegularExpression("\\s+")).value(1).toLongLong();
            }
            if (totalKb > 0) {
                qint64 usedMb = (totalKb - availKb) / 1024;
                qint64 totalMb = totalKb / 1024;
                parts << "RAM: " + QString::number(usedMb) + "/" + QString::number(totalMb) + " MB";
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
            if (days > 0) up += QString::number(days) + "d ";
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
        if (de.isEmpty()) de = qEnvironmentVariable("DESKTOP_SESSION");
        if (!de.isEmpty())
            parts << "DE: " + de;
    }

    // Qt version
    parts << "Qt: " + QString::fromLatin1(qVersion());

    // NUchat version
    parts << "NUchat 1.0.0";

    return "SysInfo: " + parts.join(" | ");
}

IrcConnection *IRCConnectionManager::connectionForServer(const QString &name) const
{
    for (auto it = m_connToName.begin(); it != m_connToName.end(); ++it) {
        if (it.value() == name)
            return it.key();
    }
    return nullptr;
}
