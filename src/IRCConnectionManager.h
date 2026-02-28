#pragma once

#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVector>

class IrcConnection;
class MessageModel;
class ServerChannelModel;
class Logger;
class Settings;

class IRCConnectionManager : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentNick READ currentNick NOTIFY currentNickChanged)
  Q_PROPERTY(QString channelTopic READ channelTopic NOTIFY channelTopicChanged)
  Q_PROPERTY(
      QStringList channelUsers READ channelUsers NOTIFY channelUsersChanged)
  Q_PROPERTY(bool isAway READ isAway NOTIFY awayStateChanged)

public:
  explicit IRCConnectionManager(QObject *parent = nullptr);
  ~IRCConnectionManager();

  void setMessageModel(MessageModel *model);
  void setServerChannelModel(ServerChannelModel *model);
  void setLogger(Logger *logger);
  void setSettings(Settings *settings);

  // Connect to a server  (called from C++ or QML)
  Q_INVOKABLE void connectToServer(const QString &host, int port = 6697,
                                   bool ssl = true,
                                   const QString &nick = "NUchat_user",
                                   const QString &user = "nuchat",
                                   const QString &realname = "NUchat User",
                                   const QString &password = QString(),
                                   const QString &saslMethod = QString(),
                                   const QString &saslUser = QString(),
                                   const QString &saslPass = QString(),
                                   const QString &nickServCmd = QString(),
                                   const QString &nickServPass = QString());
  Q_INVOKABLE void disconnectFromServer(const QString &host);
  Q_INVOKABLE void disconnectAll();

  // Channel / messaging (called from QML)
  Q_INVOKABLE void joinChannel(const QString &channel,
                               const QString &key = QString());
  Q_INVOKABLE void partChannel(const QString &channel,
                               const QString &reason = QString());
  Q_INVOKABLE void closeChannel(const QString &serverName,
                                const QString &channelName);
  Q_INVOKABLE void sendMessage(const QString &target, const QString &message);
  Q_INVOKABLE void sendRawCommand(const QString &raw);
  Q_INVOKABLE void changeNick(const QString &newNick);

  // Switch active context
  Q_INVOKABLE void setActiveServer(const QString &host);
  Q_INVOKABLE void setActiveChannel(const QString &channel);

  // Per-channel message history
  Q_INVOKABLE void switchToChannel(const QString &serverName,
                                   const QString &channel);

  // Open a private query tab (creates tab if needed, switches to it)
  Q_INVOKABLE void openQuery(const QString &serverName, const QString &nick);

  // Unread / highlight state queries for the sidebar
  Q_INVOKABLE bool hasUnread(const QString &server,
                             const QString &channel) const;
  Q_INVOKABLE bool hasHighlight(const QString &server,
                                const QString &channel) const;
  Q_INVOKABLE void clearUnread(const QString &server, const QString &channel);

  // ── Ignore list ──
  Q_INVOKABLE void addIgnore(const QString &mask);
  Q_INVOKABLE void removeIgnore(const QString &mask);
  Q_INVOKABLE QStringList ignoreList() const;
  Q_INVOKABLE void clearIgnoreList();

  // ── Away log ──
  Q_INVOKABLE QVariantList awayLog() const;
  Q_INVOKABLE void clearAwayLog();
  bool isAway() const { return m_isAway; }

  // ── URL Grabber ──
  Q_INVOKABLE QVariantList grabbedUrls() const;
  Q_INVOKABLE void clearGrabbedUrls();

  IrcConnection *activeConnection() const;
  QString currentNick() const;
  QString channelTopic() const;
  QStringList channelUsers() const;

  // Channel message history
  struct ChannelKey {
    QString server;
    QString channel;
    bool operator<(const ChannelKey &o) const {
      if (server != o.server)
        return server < o.server;
      return channel < o.channel;
    }
    bool operator==(const ChannelKey &o) const {
      return server == o.server && channel == o.channel;
    }
  };

  struct StoredMessage {
    QString type;
    QString text;
    QString timestamp;
  };

signals:
  void clientAdded(IrcConnection *conn);
  void currentNickChanged(const QString &nick);
  void channelJoined(const QString &server, const QString &channel);
  void channelAdded(const QString &server, const QString &channel);
  void channelParted(const QString &server, const QString &channel);
  void serverRegistered(const QString &host);
  void channelTopicChanged(const QString &topic);
  void channelUsersChanged(const QStringList &users);
  void rawLineReceived(const QString &direction, const QString &line);
  void unreadStateChanged();
  void ignoreListChanged();
  void awayStateChanged(bool away);
  void awayLogUpdated();
  void urlGrabbed(const QString &url, const QString &nick,
                  const QString &channel);

private:
  void wireConnection(IrcConnection *conn);
  void appendToChannel(const QString &server, const QString &channel,
                       const QString &type, const QString &text);
  QString serverNameFor(IrcConnection *conn) const;
  IrcConnection *connectionForServer(const QString &name) const;
  static QString gatherSysInfo();
  bool isIgnored(const QString &nickOrMask) const;
  void extractUrls(const QString &text, const QString &nick,
                   const QString &channel);
  void executePerformCommands(const QString &server);
  void attemptReconnect(const QString &host);
  void applyProxySettings(IrcConnection *conn);
  void ensureScrollbackLoaded(const QString &server, const QString &channel);

  QVector<IrcConnection *> m_connections;
  QMap<IrcConnection *, QString> m_connToName; // conn -> display name (host)
  MessageModel *m_msgModel = nullptr;
  ServerChannelModel *m_treeModel = nullptr;
  Logger *m_logger = nullptr;
  Settings *m_settings = nullptr;

  QString m_activeServer;
  QString m_activeChannel;

  // Per-channel message history
  QMap<ChannelKey, QVector<StoredMessage>> m_history;

  // Per-channel topic and user lists
  QMap<ChannelKey, QString> m_topics;
  QMap<ChannelKey, QStringList> m_users;

  // Unread / highlight tracking
  QSet<QString> m_unread;      // "server\nchannel" keys with new messages
  QSet<QString> m_highlighted; // "server\nchannel" keys with nick mentions
  QSet<QString>
      m_scrollbackLoaded; // "server\nchannel" keys mapped to loaded scrollback
  QString unreadKey(const QString &server, const QString &channel) const {
    return server + "\n" + channel;
  }

  // ── Ignore list ──
  QStringList m_ignoreList; // nick!user@host masks (wildcards supported)

  // ── Auto-reconnect state ──
  struct ReconnectInfo {
    QString host;
    int port;
    bool ssl;
    QString nick;
    QString user;
    QString realname;
    QString password;
    QString saslMethod;
    QString saslUser;
    QString saslPass;
    QString nickServCmd;
    QString nickServPass;
    int attempts = 0;
    QTimer *timer = nullptr;
  };
  QMap<QString, ReconnectInfo> m_reconnectInfo; // host -> reconnect params
  bool m_userDisconnect = false;                // true if user typed /quit

  // ── Away log ──
  bool m_isAway = false;
  struct AwayLogEntry {
    QString timestamp;
    QString nick;
    QString channel;
    QString message;
  };
  QVector<AwayLogEntry> m_awayLog;

  // ── URL Grabber ──
  struct GrabbedUrl {
    QString url;
    QString nick;
    QString channel;
    QString timestamp;
  };
  QVector<GrabbedUrl> m_grabbedUrls;
};
