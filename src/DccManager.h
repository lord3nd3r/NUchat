#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>

class IrcConnection;

// ── Single DCC file transfer ──
class DccTransfer : public QObject {
  Q_OBJECT
public:
  enum State { Pending, Connecting, Transferring, Complete, Failed, Cancelled };
  Q_ENUM(State)
  enum Direction { Send, Receive };
  Q_ENUM(Direction)

  DccTransfer(QObject *parent = nullptr);
  ~DccTransfer();

  // ── Properties ──
  QString id;            // unique identifier
  QString nick;          // remote user
  QString fileName;      // file name
  QString filePath;      // full local path
  qint64  fileSize = 0;  // total bytes
  qint64  transferred = 0;
  State   state = Pending;
  Direction direction = Receive;
  double  speed = 0.0;   // bytes/sec (moving average)
  QString errorString;

  // Network info (for incoming offers)
  quint32 peerAddress = 0;  // 32-bit IP (network byte order)
  quint16 peerPort = 0;
  int     token = 0;        // passive DCC token (0 = active)

  // ── Methods ──
  void startReceive(const QString &downloadDir);
  void startSend(const QString &localPath, quint16 listenPort);
  void cancel();
  double progress() const;
  QString speedString() const;
  QString etaString() const;
  QString sizeString() const;
  QString statusString() const;
  static QString formatSize(qint64 bytes);

signals:
  void stateChanged();
  void progressUpdated();

private slots:
  void onConnected();
  void onDataReady();
  void onSendDataReady();
  void onNewConnection();
  void onDisconnected();
  void onError(QAbstractSocket::SocketError err);
  void updateSpeed();


private:
  QTcpSocket *m_socket = nullptr;
  QTcpServer *m_server = nullptr;  // for sending
  QFile       m_file;
  QTimer      m_speedTimer;
  QElapsedTimer m_elapsed;
  qint64      m_lastBytes = 0;     // for speed calculation
  static constexpr int CHUNK_SIZE = 8192;

  void sendNextChunk();
  void sendAck();
};

// ── DCC Manager — coordinates all transfers, exposed to QML ──
class DccManager : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    NickRole,
    FileNameRole,
    FileSizeRole,
    TransferredRole,
    ProgressRole,
    StatusRole,
    SpeedRole,
    EtaRole,
    DirectionRole
  };

  explicit DccManager(QObject *parent = nullptr);

  // QAbstractListModel
  int rowCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  int count() const { return m_transfers.size(); }

  // ── Public API ──
  Q_INVOKABLE void acceptTransfer(const QString &id);
  Q_INVOKABLE void cancelTransfer(const QString &id);
  Q_INVOKABLE void clearFinished();
  Q_INVOKABLE void sendFile(const QString &nick, const QString &filePath);

  // Called from CTCP handler when DCC SEND is received
  void handleDccSend(const QString &nick, const QString &fileName,
                     quint32 ip, quint16 port, qint64 size, int token = 0);

  // Settings
  void setDownloadDir(const QString &dir) { m_downloadDir = dir; }
  void setAutoAccept(bool v) { m_autoAccept = v; }
  void setMaxFileSize(qint64 bytes) { m_maxFileSize = bytes; }
  void setConnection(IrcConnection *conn) { m_connection = conn; }

  QString downloadDir() const { return m_downloadDir; }

signals:
  void countChanged();
  void transferAdded(const QString &nick, const QString &fileName, qint64 size);
  void transferComplete(const QString &nick, const QString &fileName);
  void transferFailed(const QString &nick, const QString &fileName, const QString &error);

private:
  QVector<DccTransfer *> m_transfers;
  QString m_downloadDir;
  bool m_autoAccept = false;
  qint64 m_maxFileSize = 100 * 1024 * 1024; // 100MB default
  IrcConnection *m_connection = nullptr;

  DccTransfer *findTransfer(const QString &id);
  void refreshRow(DccTransfer *t);
};
