#include "IrcConnection.h"
#include <QDebug>

IrcConnection::IrcConnection(QObject *parent)
    : QObject(parent)
    , m_socket(new QSslSocket(this))
    , m_port(6697)
    , m_useSsl(true)
    , m_nickname("NUchat_user")
    , m_username("nuchat")
    , m_realname("NUchat User")
    , m_registered(false)
    , m_namesInProgress(false)
{
    connect(m_socket, &QSslSocket::readyRead, this, &IrcConnection::onReadyRead);
    connect(m_socket, &QSslSocket::disconnected, this, &IrcConnection::onDisconnectedSlot);
    connect(m_socket, qOverload<const QList<QSslError>&>(&QSslSocket::sslErrors),
            this, &IrcConnection::onSslErrors);
}

IrcConnection::~IrcConnection() = default;

void IrcConnection::connectToServer(const QString &host, quint16 port, bool useSsl)
{
    m_host = host;
    m_port = port;
    m_useSsl = useSsl;
    m_registered = false;
    m_readBuffer.clear();

    if (m_socket->isOpen())
        m_socket->disconnectFromHost();

    if (useSsl) {
        // Connect the encrypted signal for SSL
        connect(m_socket, &QSslSocket::encrypted, this, &IrcConnection::onSocketConnected,
                Qt::UniqueConnection);
        m_socket->connectToHostEncrypted(host, port);
    } else {
        connect(m_socket, &QSslSocket::connected, this, &IrcConnection::onSocketConnected,
                Qt::UniqueConnection);
        m_socket->connectToHost(host, port);
    }
    qDebug() << "[IRC] Connecting to" << host << ":" << port << (useSsl ? "(SSL)" : "(plain)");
}

void IrcConnection::disconnectFromServer(const QString &quitMsg)
{
    if (m_socket->isOpen()) {
        sendRaw("QUIT :" + quitMsg);
        m_socket->flush();
        m_socket->disconnectFromHost();
    }
}

void IrcConnection::sendRaw(const QString &line)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState ||
        m_socket->state() == QAbstractSocket::BoundState) {
        QByteArray data = line.toUtf8();
        data.append("\r\n");
        m_socket->write(data);
        m_socket->flush();
        qDebug() << "[IRC >]" << line;
    }
}

void IrcConnection::setNickname(const QString &nick)
{
    if (m_nickname != nick) {
        m_nickname = nick;
        emit nicknameChanged(nick);
    }
}

void IrcConnection::setUser(const QString &user, const QString &realname)
{
    m_username = user;
    m_realname = realname;
}

void IrcConnection::setPassword(const QString &pass)
{
    m_password = pass;
}

void IrcConnection::setSaslAuth(const QString &method, const QString &user, const QString &pass)
{
    m_saslMethod = method;
    m_saslUser = user;
    m_saslPass = pass;
}

void IrcConnection::setNickServCmd(const QString &cmd)
{
    m_nickServCmd = cmd;
}

void IrcConnection::setNickServPass(const QString &pass)
{
    m_nickServPass = pass;
}

void IrcConnection::setProxy(QNetworkProxy::ProxyType type,
                              const QString &host, quint16 port,
                              const QString &user, const QString &password)
{
    if (type == QNetworkProxy::NoProxy) {
        m_socket->setProxy(QNetworkProxy::NoProxy);
    } else {
        QNetworkProxy proxy(type, host, port, user, password);
        m_socket->setProxy(proxy);
    }
    qDebug() << "[IRC] Proxy set:" << static_cast<int>(type) << host << port;
}

void IrcConnection::joinChannel(const QString &channel, const QString &key)
{
    if (key.isEmpty())
        sendRaw("JOIN " + channel);
    else
        sendRaw("JOIN " + channel + " " + key);
}

void IrcConnection::partChannel(const QString &channel, const QString &reason)
{
    if (reason.isEmpty())
        sendRaw("PART " + channel);
    else
        sendRaw("PART " + channel + " :" + reason);
}

void IrcConnection::sendMessage(const QString &target, const QString &message)
{
    sendRaw(QString("PRIVMSG %1 :%2").arg(target, message));
}

void IrcConnection::sendNotice(const QString &target, const QString &notice)
{
    sendRaw(QString("NOTICE %1 :%2").arg(target, notice));
}

void IrcConnection::changeNick(const QString &newNick)
{
    sendRaw("NICK " + newNick);
}

void IrcConnection::setAway(const QString &reason)
{
    sendRaw("AWAY :" + reason);
}

void IrcConnection::setBack()
{
    sendRaw("AWAY");
}

void IrcConnection::sendCtcp(const QString &target, const QString &command, const QString &args)
{
    QString msg = "\x01" + command;
    if (!args.isEmpty())
        msg += " " + args;
    msg += "\x01";
    sendMessage(target, msg);
}

void IrcConnection::who(const QString &mask)
{
    sendRaw("WHO " + mask);
}

void IrcConnection::whois(const QString &nick)
{
    sendRaw("WHOIS " + nick);
}

// ── Slots ──

void IrcConnection::onReadyRead()
{
    m_readBuffer += m_socket->readAll();

    // Process complete lines (terminated by \r\n or \n)
    while (true) {
        int idx = m_readBuffer.indexOf('\n');
        if (idx < 0) break;
        QByteArray rawLine = m_readBuffer.left(idx);
        m_readBuffer = m_readBuffer.mid(idx + 1);
        // strip trailing \r
        if (rawLine.endsWith('\r'))
            rawLine.chop(1);
        QString line = QString::fromUtf8(rawLine);
        if (!line.isEmpty())
            processLine(line);
    }
}

void IrcConnection::onSocketConnected()
{
    qDebug() << "[IRC] Socket connected to" << m_host;
    emit socketConnected();
    sendRegistration();
}

void IrcConnection::onDisconnectedSlot()
{
    bool wasRegistered = m_registered;
    m_registered = false;
    if (wasRegistered)
        emit connectionStateChanged(false);
    emit disconnectedFromServer();
    qDebug() << "[IRC] Disconnected from" << m_host;
}

void IrcConnection::onSslErrors(const QList<QSslError> &errors)
{
    QStringList msgs;
    for (const QSslError &e : errors)
        msgs << e.errorString();
    qDebug() << "[IRC] SSL errors:" << msgs.join(", ") << "- ignoring";
    // Accept all SSL errors (self-signed certs, etc.)
    m_socket->ignoreSslErrors();
}

// ── IRC Protocol ──

void IrcConnection::sendRegistration()
{
    // CAP negotiation — request sasl if configured
    sendRaw("CAP LS 302");

    if (!m_password.isEmpty())
        sendRaw("PASS " + m_password);

    sendRaw("NICK " + m_nickname);
    sendRaw("USER " + m_username + " 0 * :" + m_realname);
}

QString IrcConnection::nickFromPrefix(const QString &prefix)
{
    int bang = prefix.indexOf('!');
    return (bang > 0) ? prefix.left(bang) : prefix;
}

void IrcConnection::processLine(const QString &line)
{
    emit rawLineReceived(line);
    qDebug() << "[IRC <]" << line;

    // PING/PONG (no prefix)
    if (line.startsWith("PING ")) {
        sendRaw("PONG " + line.mid(5));
        return;
    }

    // Parse: [:prefix] command [params...] [:trailing]
    QString prefix, command, trailing;
    QStringList params;
    QString rest = line;

    if (rest.startsWith(':')) {
        int sp = rest.indexOf(' ');
        if (sp < 0) return;
        prefix = rest.mid(1, sp - 1);
        rest = rest.mid(sp + 1);
    }

    // Find trailing
    int colonIdx = rest.indexOf(" :");
    if (colonIdx >= 0) {
        trailing = rest.mid(colonIdx + 2);
        rest = rest.left(colonIdx);
    }

    QStringList tokens = rest.split(' ', Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return;
    command = tokens.takeFirst().toUpper();
    params = tokens;
    if (!trailing.isNull())
        params.append(trailing);

    // ── Handle by command ──

    // CAP negotiation
    if (command == "CAP") {
        // params: [nick] [subcommand] ... [trailing]
        QString sub = (params.size() >= 2) ? params[1].toUpper() : "";
        if (sub == "LS") {
            // Check if server supports sasl and we want it
            QString capList = params.last();
            bool serverHasSasl = capList.contains("sasl", Qt::CaseInsensitive);
            bool wantSasl = !m_saslMethod.isEmpty() && m_saslMethod != "None";

            if (serverHasSasl && wantSasl) {
                m_saslInProgress = true;
                sendRaw("CAP REQ :sasl");
                qDebug() << "[IRC] Requesting SASL capability";
            } else {
                sendRaw("CAP END");
            }
        } else if (sub == "ACK") {
            QString acked = params.last();
            if (acked.contains("sasl", Qt::CaseInsensitive)) {
                // Start SASL authentication
                if (m_saslMethod == "PLAIN") {
                    sendRaw("AUTHENTICATE PLAIN");
                } else if (m_saslMethod == "EXTERNAL") {
                    sendRaw("AUTHENTICATE EXTERNAL");
                } else {
                    // Unsupported method, just end
                    qDebug() << "[IRC] Unsupported SASL method:" << m_saslMethod;
                    m_saslInProgress = false;
                    sendRaw("CAP END");
                }
            } else {
                sendRaw("CAP END");
            }
        } else if (sub == "NAK") {
            m_saslInProgress = false;
            sendRaw("CAP END");
        }
        return;
    }

    // AUTHENTICATE challenge
    if (command == "AUTHENTICATE") {
        if (params.size() >= 1 && params[0] == "+") {
            if (m_saslMethod == "PLAIN") {
                // SASL PLAIN: base64(authzid \0 authcid \0 password)
                QString authStr = m_saslUser + QChar('\0') + m_saslUser + QChar('\0') + m_saslPass;
                QByteArray encoded = authStr.toUtf8().toBase64();
                sendRaw("AUTHENTICATE " + QString::fromLatin1(encoded));
                qDebug() << "[IRC] Sent SASL PLAIN credentials for" << m_saslUser;
            } else if (m_saslMethod == "EXTERNAL") {
                sendRaw("AUTHENTICATE +");
                qDebug() << "[IRC] Sent SASL EXTERNAL (empty)";
            }
        }
        return;
    }

    // Numeric replies
    bool isNumeric = false;
    int numeric = command.toInt(&isNumeric);

    if (isNumeric) {
        // Strip the first param (our nick) for the signal
        QStringList numParams = params;
        if (!numParams.isEmpty()) numParams.removeFirst(); // remove our nick
        QString numTrailing = numParams.isEmpty() ? QString() : numParams.last();

        emit numericReceived(numeric, numParams, numTrailing);

        // RPL_WELCOME (001) — registration complete
        if (numeric == 1) {
            m_registered = true;
            m_saslInProgress = false;
            // Update our nick from the server's response (params[0] is our actual nick)
            if (!params.isEmpty()) {
                QString actualNick = params.first();
                if (actualNick != m_nickname) {
                    m_nickname = actualNick;
                    emit nicknameChanged(m_nickname);
                }
            }
            emit registered();
            emit connectionStateChanged(true);
            qDebug() << "[IRC] Registered as" << m_nickname << "on" << m_host;

            // Auto-identify via NickServ if configured
            if (!m_nickServPass.isEmpty()) {
                QString cmd = m_nickServCmd;
                if (cmd.isEmpty())
                    cmd = "/msg NickServ IDENTIFY %p";
                cmd.replace("%p", m_nickServPass);
                cmd.replace("%n", m_nickname);
                // Parse and send the command
                if (cmd.startsWith("/msg ", Qt::CaseInsensitive)) {
                    QString rest = cmd.mid(5).trimmed();
                    QString target = rest.section(' ', 0, 0);
                    QString msg = rest.section(' ', 1);
                    if (!target.isEmpty() && !msg.isEmpty())
                        sendMessage(target, msg);
                } else if (cmd.startsWith("/nickserv ", Qt::CaseInsensitive)) {
                    QString msg = cmd.mid(10).trimmed();
                    if (!msg.isEmpty())
                        sendMessage("NickServ", msg);
                } else if (cmd.startsWith("/")) {
                    sendRaw(cmd.mid(1));
                }
                qDebug() << "[IRC] Sent NickServ auto-identify for" << m_nickname;
            }
        }
        // RPL_SASLSUCCESS (903)
        else if (numeric == 903) {
            m_saslInProgress = false;
            qDebug() << "[IRC] SASL authentication successful";
            sendRaw("CAP END");
        }
        // ERR_SASLFAIL (904), ERR_SASLTOOLONG (905), ERR_SASLABORTED (906), ERR_SASLALREADY (907)
        else if (numeric == 904 || numeric == 905 || numeric == 906 || numeric == 907) {
            m_saslInProgress = false;
            qDebug() << "[IRC] SASL authentication failed (" << numeric << ")";
            emit errorOccurred("SASL authentication failed (" + QString::number(numeric) + "): " + numTrailing);
            sendRaw("CAP END");
        }
        // RPL_TOPIC (332)
        else if (numeric == 332 && numParams.size() >= 2) {
            emit topicReceived(numParams[0], numParams[1]);
        }
        // RPL_TOPICWHOTIME (333)
        else if (numeric == 333 && numParams.size() >= 3) {
            emit topicWhoTimeReceived(numParams[0], numParams[1], numParams[2]);
        }
        // RPL_NAMREPLY (353)
        else if (numeric == 353) {
            // params after strip: [=/*/@] #channel :names
            // numParams might be: ["=", "#channel", "nick1 nick2 ..."]
            // or: ["#channel", "nick1 nick2 ..."]
            QString channel;
            QString namesList;
            if (numParams.size() >= 3) {
                channel = numParams[1];
                namesList = numParams[2];
            } else if (numParams.size() >= 2) {
                channel = numParams[0];
                namesList = numParams[1];
            }
            if (!channel.isEmpty()) {
                QStringList names = namesList.split(' ', Qt::SkipEmptyParts);
                m_channelNames[channel] += names;
            }
        }
        // RPL_ENDOFNAMES (366)
        else if (numeric == 366) {
            QString channel = numParams.isEmpty() ? "" : numParams[0];
            if (m_channelNames.contains(channel)) {
                emit namesReceived(channel, m_channelNames[channel]);
                m_channelNames.remove(channel);
            }
        }
        // ERR_NICKNAMEINUSE (433)
        else if (numeric == 433) {
            // Try appending underscore
            m_nickname += "_";
            sendRaw("NICK " + m_nickname);
            emit nicknameChanged(m_nickname);
        }
        return;
    }

    // ── Named commands ──

    if (command == "PRIVMSG") {
        if (params.size() < 2) return;
        QString target = params[0];
        QString msg = params[1];
        // Check for CTCP
        if (msg.startsWith('\x01') && msg.endsWith('\x01')) {
            QString ctcp = msg.mid(1, msg.length() - 2);
            int sp = ctcp.indexOf(' ');
            QString ctcpCmd = (sp >= 0) ? ctcp.left(sp).toUpper() : ctcp.toUpper();
            QString ctcpArgs = (sp >= 0) ? ctcp.mid(sp + 1) : "";
            // Auto-reply to common CTCPs
            QString nick = nickFromPrefix(prefix);
            if (ctcpCmd == "VERSION") {
                sendRaw("NOTICE " + nick + " :\x01VERSION NUchat 0.1 (Qt6)\x01");
            } else if (ctcpCmd == "PING") {
                sendRaw("NOTICE " + nick + " :\x01PING " + ctcpArgs + "\x01");
            } else if (ctcpCmd == "TIME") {
                sendRaw("NOTICE " + nick + " :\x01TIME " + QDateTime::currentDateTime().toString() + "\x01");
            }
            emit ctcpReceived(prefix, target, ctcpCmd, ctcpArgs);
            return;
        }
        emit privmsgReceived(prefix, target, msg);
    }
    else if (command == "NOTICE") {
        if (params.size() < 2) return;
        emit noticeReceived(prefix, params[0], params[1]);
    }
    else if (command == "JOIN") {
        QString channel = params.isEmpty() ? "" : params[0];
        emit joinReceived(prefix, channel);
        // If it's us joining, init names list
        if (nickFromPrefix(prefix) == m_nickname) {
            m_channelNames[channel] = QStringList();
        }
    }
    else if (command == "PART") {
        QString channel = params.isEmpty() ? "" : params[0];
        QString reason = (params.size() >= 2) ? params[1] : "";
        emit partReceived(prefix, channel, reason);
    }
    else if (command == "QUIT") {
        QString reason = params.isEmpty() ? "" : params[0];
        emit quitReceived(prefix, reason);
    }
    else if (command == "KICK") {
        if (params.size() < 2) return;
        QString channel = params[0];
        QString kicked = params[1];
        QString reason = (params.size() >= 3) ? params[2] : "";
        emit kickReceived(prefix, channel, kicked, reason);
    }
    else if (command == "NICK") {
        QString newNick = params.isEmpty() ? "" : params[0];
        QString oldNick = nickFromPrefix(prefix);
        if (oldNick == m_nickname) {
            m_nickname = newNick;
            emit nicknameChanged(m_nickname);
        }
        emit nickChanged(oldNick, newNick);
    }
    else if (command == "TOPIC") {
        if (params.size() >= 2)
            emit topicReceived(params[0], params[1]);
    }
    else if (command == "MODE") {
        if (params.size() >= 2) {
            QString target = params[0];
            QString modeStr = params[1];
            QStringList modeParams = params.mid(2);
            emit modeReceived(prefix, target, modeStr, modeParams);
        }
    }
    else if (command == "ERROR") {
        emit errorOccurred(params.isEmpty() ? "Unknown error" : params.join(" "));
    }
}
