#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <functional>

class IrcConnection;
class MessageModel;
class ServerChannelModel;
class Logger;
class Settings;
class DccManager;

class IRCConnectionManager : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString currentNick READ currentNick NOTIFY currentNickChanged)
  Q_PROPERTY(QString channelTopic READ channelTopic NOTIFY channelTopicChanged)
  Q_PROPERTY(
      QStringList channelUsers READ channelUsers NOTIFY channelUsersChanged)
  Q_PROPERTY(bool isAway READ isAway NOTIFY awayStateChanged)
  Q_PROPERTY(int lagMs READ lagMs NOTIFY lagChanged)

public:
  explicit IRCConnectionManager(QObject *parent = nullptr);
  ~IRCConnectionManager();

  void setMessageModel(MessageModel *model);
  void setServerChannelModel(ServerChannelModel *model);
  void setLogger(Logger *logger);
  void setSettings(Settings *settings);
  void setDccManager(DccManager *dcc) { m_dccManager = dcc; }
  int lagMs() const { return m_lagMs; }

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
                                   const QString &nickServPass = QString(),
                                   const QString &autojoin = QString(),
                                   const QString &perform = QString(),
                                   const QString &displayName = QString());
  Q_INVOKABLE void disconnectFromServer(const QString &host);
  Q_INVOKABLE void disconnectAll();

  // Channel / messaging (called from QML)
  Q_INVOKABLE void joinChannel(const QString &channel,
                               const QString &key = QString());
  Q_INVOKABLE void partChannel(const QString &channel,
                               const QString &reason = QString());
  Q_INVOKABLE void closeChannel(const QString &serverName,
                                const QString &channelName);
  Q_INVOKABLE void closeServer(const QString &serverName);
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
  Q_INVOKABLE bool anyUnread() const { return !m_unread.isEmpty(); }
  Q_INVOKABLE bool anyHighlight() const { return !m_highlighted.isEmpty(); }

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
  // Raw topic text (mIRC control codes intact) — for inline topic editing
  Q_INVOKABLE QString channelTopicRaw() const;
  QStringList channelUsers() const;
  Q_INVOKABLE QString channelModes() const;

  // Channel message history
  struct ChannelKey {
    QString server;
    QString channel;
    bool operator<(const ChannelKey &o) const {
      int cmp = server.compare(o.server, Qt::CaseInsensitive);
      if (cmp != 0)
        return cmp < 0;
      return channel.compare(o.channel, Qt::CaseInsensitive) < 0;
    }
    bool operator==(const ChannelKey &o) const {
      return server.compare(o.server, Qt::CaseInsensitive) == 0 &&
             channel.compare(o.channel, Qt::CaseInsensitive) == 0;
    }
    friend size_t qHash(const ChannelKey &k, size_t seed = 0) {
      return qHash(k.server.toLower(), seed) ^ qHash(k.channel.toLower(), seed);
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
  void serverClosed(const QString &serverName);
  void channelTopicChanged(const QString &topic);
  void channelUsersChanged(const QStringList &users);
  void channelModesChanged(const QString &modes);
  void rawLineReceived(const QString &direction, const QString &line);
  void unreadStateChanged();
  void ignoreListChanged();
  void awayStateChanged(bool away);
  void awayLogUpdated();
  void urlGrabbed(const QString &url, const QString &nick,
                  const QString &channel);
  // Desktop notification trigger: emitted for highlights/PMs on non-active channels
  void notifyUser(const QString &title, const QString &message,
                  bool isHighlight, bool isPrivate);
  void lagChanged(int ms);
  void banListEntry(const QString &channel, const QString &mask,
                    const QString &setBy, const QString &timestamp);
  void banListEnd(const QString &channel);

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
  void cleanupChannelState(const QString &server, const QString &channel);
  // ── Command dispatch table ──
  // Each handler receives (conn, target, args) and returns true if consumed.
  using CommandHandler = std::function<bool(IrcConnection *conn,
                                            const QString &target,
                                            const QString &args)>;
  QHash<QString, CommandHandler> m_commandTable;
  void initCommandTable();
  // Dispatch /command input — returns true if the message was consumed.
  bool handleSlashCommand(IrcConnection *conn, const QString &target,
                          const QString &cmd, const QString &args);

  QVector<IrcConnection *> m_connections;
  QMap<IrcConnection *, QString> m_connToName; // conn -> display name (host)
  MessageModel *m_msgModel = nullptr;
  ServerChannelModel *m_treeModel = nullptr;
  Logger *m_logger = nullptr;
  Settings *m_settings = nullptr;

  QString m_activeServer;
  QString m_activeChannel;

  // Per-channel message history
  static constexpr int kMaxHistoryPerChannel = 5000;
  QMap<ChannelKey, QVector<StoredMessage>> m_history;

  // Per-channel topic and user lists
  QMap<ChannelKey, QString> m_topics;
  QMap<ChannelKey, QStringList> m_users;
  QMap<ChannelKey, QString> m_modes;  // per-channel mode string (+nst etc.)

  // Unread / highlight tracking
  QSet<QString> m_unread;      // "server\nchannel" keys with new messages
  QSet<QString> m_highlighted; // "server\nchannel" keys with nick mentions
  QSet<QString>
      m_scrollbackLoaded; // "server\nchannel" keys mapped to loaded scrollback
  QString unreadKey(const QString &server, const QString &channel) const {
    return server + "\n" + channel;
  }

  // ── Structured WHOIS accumulation ──
  // Key: "server\nnick", Value: accumulated formatted lines
  QMap<QString, QStringList> m_whoisBuffer;

  // ── Ignore list ──
  QStringList m_ignoreList; // nick!user@host masks (wildcards supported)

  // ── DCC file transfer ──
  DccManager *m_dccManager = nullptr;

  // ── Lag meter ──
  // Per-connection lag tracking for multi-server support
  struct LagState {
    QElapsedTimer pingSent;
    bool pending = false;
    int lagMs = -1;
  };
  QTimer m_lagTimer;
  QMap<IrcConnection *, LagState> m_lagState;
  int m_lagMs = -1; // -1 = unknown (aggregate / active connection)

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
    QString autojoin;  // comma/space-separated channels joined on connect
    QString perform;   // newline-separated commands run on connect
    QString displayName;
    int attempts = 0;
    QTimer *timer = nullptr;
  };
  QMap<QString, ReconnectInfo> m_reconnectInfo; // host -> reconnect params
  // Hosts the user intentionally disconnected (suppresses auto-reconnect
  // for that host only — a global flag would break multi-server semantics)
  QSet<QString> m_userDisconnected;

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
  static constexpr int kMaxGrabbedUrls = 500;
  QVector<GrabbedUrl> m_grabbedUrls;

  static constexpr int kMaxAwayLogEntries = 500;
};
