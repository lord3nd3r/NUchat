#pragma once

#include <QObject>
#include <QQueue>
#include <QSet>
#include <QStringList>
#include <QMap>
#include <QTimer>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QNetworkProxy>

class IrcConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString nickname READ nickname WRITE setNickname NOTIFY nicknameChanged)
    Q_PROPERTY(QString serverHost READ serverHost NOTIFY serverHostChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)

public:
    explicit IrcConnection(QObject *parent = nullptr);
    ~IrcConnection();

    // Connection
    void connectToServer(const QString &host, quint16 port = 6697, bool useSsl = true);
    void disconnectFromServer(const QString &quitMsg = "NUchat");
    Q_INVOKABLE void sendRaw(const QString &line);

    // Strip IRC mode prefixes (~&@%+) from a nick; returns {prefix, bare_nick}
    static QPair<QString, QString> stripNickPrefix(const QString &nick);

    // Identity
    QString nickname() const { return m_nickname; }
    void setNickname(const QString &nick);
    void setUser(const QString &user, const QString &realname);
    void setPassword(const QString &pass);

    // Per-network authentication
    void setSaslAuth(const QString &method, const QString &user, const QString &pass);
    void setNickServCmd(const QString &cmd);   // e.g. "/msg NickServ IDENTIFY %p" (%p=password)
    void setNickServPass(const QString &pass);

    // Proxy configuration
    void setProxy(QNetworkProxy::ProxyType type,
                  const QString &host = QString(), quint16 port = 0,
                  const QString &user = QString(), const QString &password = QString());

    // SSL options
    void setAllowSelfSignedCerts(bool allow);

    QString serverHost() const { return m_host; }
    bool isConnected() const { return m_registered; }

    // IRC commands
    Q_INVOKABLE void joinChannel(const QString &channel, const QString &key = QString());
    Q_INVOKABLE void partChannel(const QString &channel, const QString &reason = QString());
    Q_INVOKABLE void sendMessage(const QString &target, const QString &message);
    Q_INVOKABLE void sendNotice(const QString &target, const QString &notice);
    Q_INVOKABLE void changeNick(const QString &newNick);
    Q_INVOKABLE void setAway(const QString &reason = QString());
    Q_INVOKABLE void setBack();
    Q_INVOKABLE void sendCtcp(const QString &target, const QString &command, const QString &args = QString());
    Q_INVOKABLE void who(const QString &mask);
    Q_INVOKABLE void whois(const QString &nick);

signals:
    void socketConnected();              // TCP/SSL connected (before registration)
    void registered();                   // RPL_WELCOME received — fully logged in
    void disconnectedFromServer();
    void rawLineReceived(const QString &line);
    void nicknameChanged(const QString &nick);
    void serverHostChanged(const QString &host);
    void connectionStateChanged(bool connected);

    // Parsed IRC events
    void privmsgReceived(const QString &prefix, const QString &target, const QString &message);
    void noticeReceived(const QString &prefix, const QString &target, const QString &message);
    void joinReceived(const QString &prefix, const QString &channel);
    void partReceived(const QString &prefix, const QString &channel, const QString &reason);
    void quitReceived(const QString &prefix, const QString &reason);
    void kickReceived(const QString &prefix, const QString &channel, const QString &kicked, const QString &reason);
    void nickChanged(const QString &oldNick, const QString &newNick);
    void topicReceived(const QString &channel, const QString &topic);
    void topicWhoTimeReceived(const QString &channel, const QString &who, const QString &when);
    void modeReceived(const QString &prefix, const QString &target, const QString &modeStr, const QStringList &params);
    void numericReceived(int code, const QStringList &params, const QString &trailing);
    void ctcpReceived(const QString &prefix, const QString &target, const QString &command, const QString &args);
    void namesReceived(const QString &channel, const QStringList &names);
    void errorOccurred(const QString &error);

    // IRCv3 message-tags: emitted when a tagged message with a server-time
    // is received.  tags is the parsed @tag map from the line.
    void taggedMessageReceived(const QMap<QString, QString> &tags);

private slots:
    void onReadyRead();
    void onSocketConnected();
    void onDisconnectedSlot();
    void onSslErrors(const QList<QSslError> &errors);
    void drainSendQueue();

private:
    QSslSocket *m_socket;
    QString m_host;
    quint16 m_port;
    bool m_useSsl;
    bool m_allowSelfSignedCerts = false;
    QString m_nickname;
    QString m_username;
    QString m_realname;
    QString m_password;
    bool m_registered;
    int m_nickRetries = 0;      // incremented on each 433; reset on registration
    QByteArray m_readBuffer;

    // SASL authentication state
    QString m_saslMethod;   // "None", "PLAIN", "EXTERNAL"
    QString m_saslUser;
    QString m_saslPass;
    bool m_saslInProgress = false;

    // NickServ auto-identify
    QString m_nickServCmd;  // e.g. "/msg NickServ IDENTIFY %p"
    QString m_nickServPass;

    // Channels we've joined — tracks names lists
    QMap<QString, QStringList> m_channelNames;
    QSet<QString> m_namesStarted;   // channels with an in-progress 353 sequence

    // ── Flood protection ──
    // Burst-then-throttle: allow up to kBurst messages instantly, then
    // drain the queue at kDrainIntervalMs.
    static constexpr int kBurst = 5;
    static constexpr int kDrainIntervalMs = 1000;  // 1 message per second
    QQueue<QString> m_sendQueue;
    QTimer m_sendTimer;
    int m_burstRemaining = kBurst;

    // ── Multi-line CAP LS accumulation ──
    QString m_pendingCapLs;  // caps seen so far when server sends CAP * LS *

    // ── IRCv3 message-tags ──
    QMap<QString, QString> m_lastTags;  // tags from the most recently parsed line
    static QMap<QString, QString> parseTags(const QString &tagStr);

    void processLine(const QString &line);
    void sendRawImmediate(const QString &line);  // bypass flood queue

    // Allow unit tests to drive processLine() directly
    friend class IrcConnectionTestable;
    void sendRegistration();
    static QString nickFromPrefix(const QString &prefix);
};
