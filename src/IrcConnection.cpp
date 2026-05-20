#include "IrcConnection.h"
#include "Version.h"
#include <QDebug>

IrcConnection::IrcConnection(QObject *parent)
    : QObject(parent), m_socket(new QSslSocket(this)), m_port(6697),
      m_useSsl(true), m_nickname("NUchat_user"), m_username("nuchat"),
      m_realname("NUchat User"), m_registered(false) {
  connect(m_socket, &QSslSocket::readyRead, this, &IrcConnection::onReadyRead);
  connect(m_socket, &QSslSocket::disconnected, this,
          &IrcConnection::onDisconnectedSlot);
  connect(m_socket, qOverload<const QList<QSslError> &>(&QSslSocket::sslErrors),
          this, &IrcConnection::onSslErrors);

  // Flood protection: drain one queued message per timer tick
  m_sendTimer.setInterval(kDrainIntervalMs);
  connect(&m_sendTimer, &QTimer::timeout, this, &IrcConnection::drainSendQueue);
}

IrcConnection::~IrcConnection() = default;

void IrcConnection::connectToServer(const QString &host, quint16 port,
                                    bool useSsl) {
  m_host = host;
  m_port = port;
  m_useSsl = useSsl;
  m_registered = false;
  m_readBuffer.clear();

  if (m_socket->isOpen())
    m_socket->disconnectFromHost();

  if (useSsl) {
    // Connect the encrypted signal for SSL
    connect(m_socket, &QSslSocket::encrypted, this,
            &IrcConnection::onSocketConnected, Qt::UniqueConnection);
    m_socket->connectToHostEncrypted(host, port);
  } else {
    connect(m_socket, &QSslSocket::connected, this,
            &IrcConnection::onSocketConnected, Qt::UniqueConnection);
    m_socket->connectToHost(host, port);
  }
  qDebug() << "[IRC] Connecting to" << host << ":" << port
           << (useSsl ? "(SSL)" : "(plain)");
}

void IrcConnection::disconnectFromServer(const QString &quitMsg) {
  if (m_socket->isOpen()) {
    sendRaw("QUIT :" + quitMsg);
    m_socket->flush();
    m_socket->disconnectFromHost();
  }
}

void IrcConnection::sendRaw(const QString &line) {
  if (m_socket->state() != QAbstractSocket::ConnectedState &&
      m_socket->state() != QAbstractSocket::BoundState)
    return;

  // Flood protection: burst-then-throttle.
  // Allow kBurst messages instantly, then queue the rest.
  if (m_burstRemaining > 0) {
    --m_burstRemaining;
    sendRawImmediate(line);
    if (!m_sendTimer.isActive())
      m_sendTimer.start();
  } else {
    m_sendQueue.enqueue(line);
    if (!m_sendTimer.isActive())
      m_sendTimer.start();
  }
}

void IrcConnection::sendRawImmediate(const QString &line) {
  QByteArray data = line.toUtf8();
  data.append("\r\n");
  m_socket->write(data);
  m_socket->flush();
  qDebug() << "[IRC >]" << line;
}

void IrcConnection::drainSendQueue() {
  if (m_sendQueue.isEmpty()) {
    // Nothing queued — replenish burst tokens and stop the timer
    m_burstRemaining = kBurst;
    m_sendTimer.stop();
    return;
  }
  // Send one queued message
  QString line = m_sendQueue.dequeue();
  sendRawImmediate(line);
}

QPair<QString, QString> IrcConnection::stripNickPrefix(const QString &nick) {
  static const QString prefixes = QStringLiteral("~&@%+");
  QString pfx, bare = nick;
  while (!bare.isEmpty() && prefixes.contains(bare[0])) {
    pfx += bare[0];
    bare = bare.mid(1);
  }
  return {pfx, bare};
}

void IrcConnection::setNickname(const QString &nick) {
  if (m_nickname != nick) {
    m_nickname = nick;
    emit nicknameChanged(nick);
  }
}

void IrcConnection::setUser(const QString &user, const QString &realname) {
  m_username = user;
  m_realname = realname;
}

void IrcConnection::setPassword(const QString &pass) { m_password = pass; }

void IrcConnection::setSaslAuth(const QString &method, const QString &user,
                                const QString &pass) {
  m_saslMethod = method;
  m_saslUser = user;
  m_saslPass = pass;
}

void IrcConnection::setNickServCmd(const QString &cmd) { m_nickServCmd = cmd; }

void IrcConnection::setNickServPass(const QString &pass) {
  m_nickServPass = pass;
}

void IrcConnection::setProxy(QNetworkProxy::ProxyType type, const QString &host,
                             quint16 port, const QString &user,
                             const QString &password) {
  if (type == QNetworkProxy::NoProxy) {
    m_socket->setProxy(QNetworkProxy::NoProxy);
  } else {
    QNetworkProxy proxy(type, host, port, user, password);
    m_socket->setProxy(proxy);
  }
  qDebug() << "[IRC] Proxy set:" << static_cast<int>(type) << host << port;
}

void IrcConnection::joinChannel(const QString &channel, const QString &key) {
  if (key.isEmpty())
    sendRaw("JOIN " + channel);
  else
    sendRaw("JOIN " + channel + " " + key);
}

void IrcConnection::partChannel(const QString &channel, const QString &reason) {
  if (reason.isEmpty())
    sendRaw("PART " + channel);
  else
    sendRaw("PART " + channel + " :" + reason);
}

void IrcConnection::sendMessage(const QString &target, const QString &message) {
  // IRC line limit: 512 bytes including CRLF.  The server prepends our
  // prefix (:nick!user@host) which can be up to ~70 bytes.  Leave room:
  // 512 - 2(CRLF) - 70(prefix) - len("PRIVMSG  :") = ~430 usable bytes.
  // For safety we use 400 as the max payload per chunk.
  constexpr int kMaxPayload = 400;
  QByteArray targetUtf8 = target.toUtf8();
  int overhead = QByteArray("PRIVMSG  :").size() + targetUtf8.size();
  int maxMsg = kMaxPayload - overhead;
  if (maxMsg < 50) maxMsg = 50;  // safety floor

  QByteArray msgUtf8 = message.toUtf8();
  if (msgUtf8.size() <= maxMsg) {
    sendRaw(QString("PRIVMSG %1 :%2").arg(target, message));
    return;
  }

  // Split at UTF-8 safe boundaries
  int pos = 0;
  while (pos < msgUtf8.size()) {
    int end = qMin(pos + maxMsg, (int)msgUtf8.size());
    // Don't split in the middle of a multi-byte UTF-8 sequence
    while (end > pos && (msgUtf8[end] & 0xC0) == 0x80)
      --end;
    if (end == pos) end = pos + maxMsg; // pathological: force advance
    QByteArray chunk = msgUtf8.mid(pos, end - pos);
    sendRaw(QString("PRIVMSG %1 :%2").arg(target, QString::fromUtf8(chunk)));
    pos = end;
  }
}

void IrcConnection::sendNotice(const QString &target, const QString &notice) {
  sendRaw(QString("NOTICE %1 :%2").arg(target, notice));
}

void IrcConnection::changeNick(const QString &newNick) {
  sendRaw("NICK " + newNick);
}

void IrcConnection::setAway(const QString &reason) {
  sendRaw("AWAY :" + reason);
}

void IrcConnection::setBack() { sendRaw("AWAY"); }

void IrcConnection::sendCtcp(const QString &target, const QString &command,
                             const QString &args) {
  QString msg = "\x01" + command;
  if (!args.isEmpty())
    msg += " " + args;
  msg += "\x01";
  sendMessage(target, msg);
}

void IrcConnection::who(const QString &mask) { sendRaw("WHO " + mask); }

void IrcConnection::whois(const QString &nick) { sendRaw("WHOIS " + nick); }

// ── Slots ──

void IrcConnection::onReadyRead() {
  m_readBuffer += m_socket->readAll();

  // Process complete lines (terminated by \r\n or \n)
  while (true) {
    int idx = m_readBuffer.indexOf('\n');
    if (idx < 0)
      break;
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

void IrcConnection::onSocketConnected() {
  qDebug() << "[IRC] Socket connected to" << m_host;
  emit socketConnected();
  sendRegistration();
}

void IrcConnection::onDisconnectedSlot() {
  bool wasRegistered = m_registered;
  m_registered = false;
  if (wasRegistered)
    emit connectionStateChanged(false);
  emit disconnectedFromServer();
  qDebug() << "[IRC] Disconnected from" << m_host;
}

void IrcConnection::setAllowSelfSignedCerts(bool allow) {
  m_allowSelfSignedCerts = allow;
}

void IrcConnection::onSslErrors(const QList<QSslError> &errors) {
  QList<QSslError> ignorable;
  for (const QSslError &e : errors) {
    switch (e.error()) {
    case QSslError::SelfSignedCertificate:
    case QSslError::SelfSignedCertificateInChain:
    case QSslError::HostNameMismatch:
    case QSslError::UnableToVerifyFirstCertificate:
    case QSslError::CertificateUntrusted:
    case QSslError::UnableToGetLocalIssuerCertificate:
      if (m_allowSelfSignedCerts) {
        ignorable << e;
        qDebug() << "[IRC] Ignoring SSL error (user-approved):" << e.errorString();
      } else {
        emit errorOccurred(tr("SSL error: %1").arg(e.errorString()));
        m_socket->abort();
        return;
      }
      break;
    default:
      emit errorOccurred(tr("SSL error: %1").arg(e.errorString()));
      m_socket->abort();
      return;
    }
  }
  if (!ignorable.isEmpty())
    m_socket->ignoreSslErrors(ignorable);
}

// ── IRC Protocol ──

void IrcConnection::sendRegistration() {
  // Registration messages bypass the flood queue — they must go immediately
  // or the server will time us out.
  sendRawImmediate("CAP LS 302");

  if (!m_password.isEmpty()) {
    sendRawImmediate("PASS " + m_password);
    m_password.fill('\0');
    m_password.clear();
  }

  sendRawImmediate("NICK " + m_nickname);
  sendRawImmediate("USER " + m_username + " 0 * :" + m_realname);
}

QString IrcConnection::nickFromPrefix(const QString &prefix) {
  int bang = prefix.indexOf('!');
  return (bang > 0) ? prefix.left(bang) : prefix;
}

void IrcConnection::processLine(const QString &line) {
  emit rawLineReceived(line);
  qDebug() << "[IRC <]" << line;

  // ── IRCv3 message-tags: strip @tags prefix ──
  // Format: @tag1=val;tag2;tag3=val :prefix COMMAND ...
  QString rest = line;
  m_lastTags.clear();
  if (rest.startsWith('@')) {
    int sp = rest.indexOf(' ');
    if (sp > 0) {
      m_lastTags = parseTags(rest.mid(1, sp - 1));
      rest = rest.mid(sp + 1).trimmed();
      emit taggedMessageReceived(m_lastTags);
    }
  }

  // PING/PONG (no prefix)
  if (rest.startsWith("PING ")) {
    sendRawImmediate("PONG " + rest.mid(5));  // PONG must not be queued
    return;
  }

  // Parse: [:prefix] command [params...] [:trailing]
  QString prefix, command, trailing;
  QStringList params;

  if (rest.startsWith(':')) {
    int sp = rest.indexOf(' ');
    if (sp < 0)
      return;
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
  if (tokens.isEmpty())
    return;
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
      // Multi-line CAP LS: if the second param (after nick) is "*", more
      // lines follow.  Accumulate until the final line (no "*").
      bool more = (params.size() >= 3 && params[2] == "*");
      QString capList = params.last();

      if (more) {
        // Continuation line — accumulate caps
        if (!m_pendingCapLs.isEmpty())
          m_pendingCapLs += " ";
        m_pendingCapLs += capList;
        return;  // wait for more lines
      }

      // Final line (or single-line LS)
      if (!m_pendingCapLs.isEmpty()) {
        capList = m_pendingCapLs + " " + capList;
        m_pendingCapLs.clear();
      }

      bool serverHasSasl = capList.contains("sasl", Qt::CaseInsensitive);
      bool wantSasl = !m_saslMethod.isEmpty() && m_saslMethod != "None";

      // Build list of IRCv3 capabilities to request
      QStringList capsToReq;
      if (capList.contains("server-time", Qt::CaseInsensitive))
        capsToReq << "server-time";
      if (serverHasSasl && wantSasl)
        capsToReq << "sasl";

      if (!capsToReq.isEmpty()) {
        if (capsToReq.contains("sasl"))
          m_saslInProgress = true;
        sendRawImmediate("CAP REQ :" + capsToReq.join(' '));
        qDebug() << "[IRC] Requesting capabilities:" << capsToReq;
      } else {
        sendRawImmediate("CAP END");
      }
    } else if (sub == "ACK") {
      QString acked = params.last();
      if (acked.contains("sasl", Qt::CaseInsensitive)) {
        // Start SASL authentication
        if (m_saslMethod == "PLAIN") {
          sendRawImmediate("AUTHENTICATE PLAIN");
        } else if (m_saslMethod == "EXTERNAL") {
          sendRawImmediate("AUTHENTICATE EXTERNAL");
        } else {
          // Unsupported method, just end
          qDebug() << "[IRC] Unsupported SASL method:" << m_saslMethod;
          m_saslInProgress = false;
          sendRawImmediate("CAP END");
        }
      } else {
        sendRawImmediate("CAP END");
      }
    } else if (sub == "NAK") {
      m_saslInProgress = false;
      sendRawImmediate("CAP END");
    }
    return;
  }

  // AUTHENTICATE challenge
  if (command == "AUTHENTICATE") {
    if (params.size() >= 1 && params[0] == "+") {
      if (m_saslMethod == "PLAIN") {
        // SASL PLAIN: base64(authzid \0 authcid \0 password)
        QString authStr =
            m_saslUser + QChar('\0') + m_saslUser + QChar('\0') + m_saslPass;
        QByteArray encoded = authStr.toUtf8().toBase64();
        sendRawImmediate("AUTHENTICATE " + QString::fromLatin1(encoded));
        qDebug() << "[IRC] Sent SASL PLAIN credentials for" << m_saslUser;
        // Zero credentials from memory immediately after use
        m_saslPass.fill('\0');
        m_saslPass.clear();
        m_saslUser.fill('\0');
        m_saslUser.clear();
      } else if (m_saslMethod == "EXTERNAL") {
        sendRawImmediate("AUTHENTICATE +");
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
    if (!numParams.isEmpty())
      numParams.removeFirst(); // remove our nick
    QString numTrailing = numParams.isEmpty() ? QString() : numParams.last();

    emit numericReceived(numeric, numParams, numTrailing);

    // RPL_WELCOME (001) — registration complete
    if (numeric == 1) {
      m_registered = true;
      m_saslInProgress = false;
      m_nickRetries = 0;
      // Update our nick from the server's response (params[0] is our actual
      // nick)
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
        // Zero NickServ password from memory immediately after use
        m_nickServPass.fill('\0');
        m_nickServPass.clear();
        qDebug() << "[IRC] Sent NickServ auto-identify for" << m_nickname;
      }
    }
    // RPL_SASLSUCCESS (903)
    else if (numeric == 903) {
      m_saslInProgress = false;
      qDebug() << "[IRC] SASL authentication successful";
      sendRawImmediate("CAP END");
    }
    // ERR_SASLFAIL (904), ERR_SASLTOOLONG (905), ERR_SASLABORTED (906),
    // ERR_SASLALREADY (907)
    else if (numeric == 904 || numeric == 905 || numeric == 906 ||
             numeric == 907) {
      m_saslInProgress = false;
      qDebug() << "[IRC] SASL authentication failed (" << numeric << ")";
      emit errorOccurred("SASL authentication failed (" +
                         QString::number(numeric) + "): " + numTrailing);
      sendRawImmediate("CAP END");
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
        // If this is the first 353 for a new names sequence, start fresh.
        if (!m_namesStarted.contains(channel)) {
          m_channelNames[channel] = QStringList();
          m_namesStarted.insert(channel);
        }
        m_channelNames[channel] += names;
      }
    }
    // RPL_ENDOFNAMES (366)
    else if (numeric == 366) {
      QString channel = numParams.isEmpty() ? "" : numParams[0];
      if (m_channelNames.contains(channel)) {
        emit namesReceived(channel, m_channelNames[channel]);
        m_channelNames.remove(channel);
        m_namesStarted.remove(channel);
      }
    }
    // ERR_NICKNAMEINUSE (433)
    else if (numeric == 433) {
      static const int kMaxNickRetries = 3;
      if (m_nickRetries < kMaxNickRetries) {
        ++m_nickRetries;
        m_nickname += "_";
        sendRaw("NICK " + m_nickname);
        emit nicknameChanged(m_nickname);
      } else {
        emit errorOccurred(
            tr("Nickname in use and all alternatives exhausted. "
               "Please choose a different nickname."));
      }
    }
    return;
  }

  // ── Named commands ──

  if (command == "PRIVMSG") {
    if (params.size() < 2)
      return;
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
        sendRaw("NOTICE " + nick +
                " :\x01VERSION NUchat " NUCHAT_VERSION " (Qt6)\x01");
      } else if (ctcpCmd == "PING") {
        sendRaw("NOTICE " + nick + " :\x01PING " + ctcpArgs + "\x01");
      } else if (ctcpCmd == "TIME") {
        sendRaw("NOTICE " + nick + " :\x01TIME " +
                QDateTime::currentDateTime().toString() + "\x01");
      }
      emit ctcpReceived(prefix, target, ctcpCmd, ctcpArgs);
      return;
    }
    emit privmsgReceived(prefix, target, msg);
  } else if (command == "NOTICE") {
    if (params.size() < 2)
      return;
    emit noticeReceived(prefix, params[0], params[1]);
  } else if (command == "JOIN") {
    QString channel = params.isEmpty() ? "" : params[0];
    emit joinReceived(prefix, channel);
    // If it's us joining, init names list
    if (nickFromPrefix(prefix) == m_nickname) {
      m_channelNames[channel] = QStringList();
      m_namesStarted.remove(channel);  // allow fresh tracking on next 353
    }
  } else if (command == "PART") {
    QString channel = params.isEmpty() ? "" : params[0];
    QString reason = (params.size() >= 2) ? params[1] : "";
    emit partReceived(prefix, channel, reason);
  } else if (command == "QUIT") {
    QString reason = params.isEmpty() ? "" : params[0];
    emit quitReceived(prefix, reason);
  } else if (command == "KICK") {
    if (params.size() < 2)
      return;
    QString channel = params[0];
    QString kicked = params[1];
    QString reason = (params.size() >= 3) ? params[2] : "";
    emit kickReceived(prefix, channel, kicked, reason);
  } else if (command == "NICK") {
    QString newNick = params.isEmpty() ? "" : params[0];
    QString oldNick = nickFromPrefix(prefix);
    if (oldNick == m_nickname) {
      m_nickname = newNick;
      emit nicknameChanged(m_nickname);
    }
    emit nickChanged(oldNick, newNick);
  } else if (command == "TOPIC") {
    if (params.size() >= 2)
      emit topicReceived(params[0], params[1]);
  } else if (command == "MODE") {
    if (params.size() >= 2) {
      QString target = params[0];
      QString modeStr = params[1];
      QStringList modeParams = params.mid(2);
      emit modeReceived(prefix, target, modeStr, modeParams);
    }
  } else if (command == "ERROR") {
    emit errorOccurred(params.isEmpty() ? "Unknown error" : params.join(" "));
  }
}

// ── IRCv3 message-tags parser ──
// Parse "tag1=val;tag2;tag3=escaped\\svalue" into QMap
QMap<QString, QString> IrcConnection::parseTags(const QString &tagStr) {
  QMap<QString, QString> tags;
  const auto parts = tagStr.split(';', Qt::SkipEmptyParts);
  for (const QString &part : parts) {
    int eq = part.indexOf('=');
    if (eq >= 0) {
      QString key = part.left(eq);
      QString val = part.mid(eq + 1);
      // IRCv3 tag value unescaping: \: = ; \s = space \\ = \ \r \n
      val.replace("\\:", ";");
      val.replace("\\s", " ");
      val.replace("\\r", "\r");
      val.replace("\\n", "\n");
      val.replace("\\\\", "\\");
      tags[key] = val;
    } else {
      tags[part] = QString();  // boolean tag (no value)
    }
  }
  return tags;
}
