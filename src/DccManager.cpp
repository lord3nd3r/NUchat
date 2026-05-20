#include "DccManager.h"
#include "IrcConnection.h"
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QUuid>
#include <QtEndian>

// ════════════════════════════════════════════════════════
//  DccTransfer
// ════════════════════════════════════════════════════════

DccTransfer::DccTransfer(QObject *parent) : QObject(parent) {
  id = QUuid::createUuid().toString(QUuid::Id128).left(8);
  m_speedTimer.setInterval(1000);
  connect(&m_speedTimer, &QTimer::timeout, this, &DccTransfer::updateSpeed);
}

DccTransfer::~DccTransfer() {
  if (m_file.isOpen()) m_file.close();
  delete m_socket;
  delete m_server;
}

double DccTransfer::progress() const {
  if (fileSize <= 0) return 0.0;
  return static_cast<double>(transferred) / static_cast<double>(fileSize);
}

QString DccTransfer::formatSize(qint64 bytes) {
  if (bytes < 1024) return QString::number(bytes) + " B";
  if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
  if (bytes < 1024LL * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
  return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

QString DccTransfer::sizeString() const { return formatSize(fileSize); }

QString DccTransfer::speedString() const {
  if (state != Transferring || speed < 1.0) return "";
  return formatSize(static_cast<qint64>(speed)) + "/s";
}

QString DccTransfer::etaString() const {
  if (state != Transferring || speed < 1.0 || fileSize <= 0) return "";
  qint64 remaining = fileSize - transferred;
  int secs = static_cast<int>(remaining / speed);
  if (secs < 60) return QString::number(secs) + "s";
  if (secs < 3600) return QString::number(secs / 60) + "m " + QString::number(secs % 60) + "s";
  return QString::number(secs / 3600) + "h " + QString::number((secs % 3600) / 60) + "m";
}

QString DccTransfer::statusString() const {
  switch (state) {
    case Pending: return "Pending";
    case Connecting: return "Connecting";
    case Transferring: return QString::number(static_cast<int>(progress() * 100)) + "%";
    case Complete: return "Complete";
    case Failed: return "Failed";
    case Cancelled: return "Cancelled";
  }
  return "Unknown";
}

void DccTransfer::updateSpeed() {
  qint64 delta = transferred - m_lastBytes;
  m_lastBytes = transferred;
  // Exponential moving average (α=0.3)
  speed = speed * 0.7 + delta * 0.3;
  emit progressUpdated();
}

// ── Receive ──
void DccTransfer::startReceive(const QString &downloadDir) {
  if (state != Pending) return;

  // Sanitize filename
  QString safeName = QFileInfo(fileName).fileName();
  safeName.replace("..", "_").replace("/", "_").replace("\\", "_");
  filePath = downloadDir + "/" + safeName;

  // Avoid overwriting: append (1), (2), etc.
  if (QFile::exists(filePath)) {
    QString base = QFileInfo(filePath).completeBaseName();
    QString ext = QFileInfo(filePath).suffix();
    int n = 1;
    while (QFile::exists(filePath)) {
      filePath = downloadDir + "/" + base + "(" + QString::number(n++) + ")" +
                 (ext.isEmpty() ? "" : "." + ext);
    }
  }

  m_file.setFileName(filePath);
  if (!m_file.open(QIODevice::WriteOnly)) {
    state = Failed;
    errorString = "Cannot open file for writing: " + m_file.errorString();
    emit stateChanged();
    return;
  }

  state = Connecting;
  emit stateChanged();

  m_socket = new QTcpSocket(this);
  connect(m_socket, &QTcpSocket::connected, this, &DccTransfer::onConnected);
  connect(m_socket, &QTcpSocket::readyRead, this, &DccTransfer::onDataReady);
  connect(m_socket, &QTcpSocket::disconnected, this, &DccTransfer::onDisconnected);
  connect(m_socket, &QTcpSocket::errorOccurred, this, &DccTransfer::onError);

  // Convert 32-bit int IP to QHostAddress
  QHostAddress addr(peerAddress);
  m_socket->connectToHost(addr, peerPort);

  // 30-second connection timeout
  QTimer::singleShot(30000, this, [this]() {
    if (state == Connecting) {
      state = Failed;
      errorString = "Connection timed out (30s)";
      if (m_socket) m_socket->abort();
      if (m_file.isOpen()) m_file.close();
      emit stateChanged();
    }
  });
}

void DccTransfer::onConnected() {
  state = Transferring;
  m_elapsed.start();
  m_lastBytes = 0;
  m_speedTimer.start();
  emit stateChanged();
}

void DccTransfer::onDataReady() {
  while (m_socket->bytesAvailable() > 0) {
    QByteArray data = m_socket->read(CHUNK_SIZE);
    m_file.write(data);
    transferred += data.size();
  }

  // Send DCC acknowledgment (4-byte big-endian total received)
  sendAck();

  if (fileSize > 0 && transferred >= fileSize) {
    state = Complete;
    m_speedTimer.stop();
    m_file.close();
    m_socket->disconnectFromHost();
    emit stateChanged();
  }

  emit progressUpdated();
}

void DccTransfer::sendAck() {
  if (!m_socket || !m_socket->isWritable()) return;
  quint32 ack = qToBigEndian(static_cast<quint32>(transferred & 0xFFFFFFFF));
  m_socket->write(reinterpret_cast<const char *>(&ack), 4);
}

// ── Send ──
void DccTransfer::startSend(const QString &localPath, quint16 listenPort) {
  filePath = localPath;
  m_file.setFileName(localPath);
  if (!m_file.open(QIODevice::ReadOnly)) {
    state = Failed;
    errorString = "Cannot open file: " + m_file.errorString();
    emit stateChanged();
    return;
  }
  fileSize = m_file.size();

  m_server = new QTcpServer(this);
  connect(m_server, &QTcpServer::newConnection, this, &DccTransfer::onNewConnection);

  if (!m_server->listen(QHostAddress::Any, listenPort)) {
    state = Failed;
    errorString = "Cannot listen on port " + QString::number(listenPort) + ": " + m_server->errorString();
    emit stateChanged();
    return;
  }

  peerPort = m_server->serverPort();
  state = Pending; // waiting for remote to connect
  emit stateChanged();
}

void DccTransfer::onNewConnection() {
  m_socket = m_server->nextPendingConnection();
  m_server->close(); // only accept one connection

  connect(m_socket, &QTcpSocket::readyRead, this, &DccTransfer::onSendDataReady);
  connect(m_socket, &QTcpSocket::disconnected, this, &DccTransfer::onDisconnected);
  connect(m_socket, &QTcpSocket::errorOccurred, this, &DccTransfer::onError);
  connect(m_socket, &QTcpSocket::bytesWritten, this, [this]() {
    if (state == Transferring) sendNextChunk();
  });

  state = Transferring;
  m_elapsed.start();
  m_lastBytes = 0;
  m_speedTimer.start();
  emit stateChanged();
  sendNextChunk();
}

void DccTransfer::sendNextChunk() {
  if (!m_socket || !m_file.isOpen() || state != Transferring) return;
  // Don't flood the socket buffer
  if (m_socket->bytesToWrite() > CHUNK_SIZE * 4) return;

  QByteArray data = m_file.read(CHUNK_SIZE);
  if (data.isEmpty()) return; // EOF — wait for ack

  m_socket->write(data);
  transferred += data.size();
  emit progressUpdated();

  if (transferred >= fileSize) {
    // All data sent, wait for remote to close or ack
  }
}

void DccTransfer::onSendDataReady() {
  // Read acknowledgment from receiver (4-byte big-endian)
  while (m_socket && m_socket->bytesAvailable() >= 4) {
    char ackBuf[4];
    m_socket->read(ackBuf, 4);
    quint32 acked = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(ackBuf));
    if (acked >= static_cast<quint32>(fileSize & 0xFFFFFFFF) && transferred >= fileSize) {
      state = Complete;
      m_speedTimer.stop();
      m_file.close();
      m_socket->disconnectFromHost();
      emit stateChanged();
      return;
    }
  }
}

void DccTransfer::cancel() {
  if (state == Complete || state == Cancelled) return;
  state = Cancelled;
  m_speedTimer.stop();
  if (m_socket) m_socket->abort();
  if (m_server) m_server->close();
  if (m_file.isOpen()) {
    m_file.close();
    // Delete partial download
    if (direction == Receive && !filePath.isEmpty())
      QFile::remove(filePath);
  }
  emit stateChanged();
}

void DccTransfer::onDisconnected() {
  if (state == Transferring) {
    if (fileSize > 0 && transferred >= fileSize) {
      state = Complete;
    } else {
      state = Failed;
      errorString = "Connection closed prematurely";
    }
    m_speedTimer.stop();
    if (m_file.isOpen()) m_file.close();
    emit stateChanged();
  }
}

void DccTransfer::onError(QAbstractSocket::SocketError) {
  if (state == Complete || state == Cancelled) return;
  state = Failed;
  errorString = m_socket ? m_socket->errorString() : "Socket error";
  m_speedTimer.stop();
  if (m_file.isOpen()) m_file.close();
  emit stateChanged();
}

// ════════════════════════════════════════════════════════
//  DccManager
// ════════════════════════════════════════════════════════

DccManager::DccManager(QObject *parent) : QAbstractListModel(parent) {
  m_downloadDir = QDir::homePath() + "/Downloads";
}

int DccManager::rowCount(const QModelIndex &) const {
  return m_transfers.size();
}

QHash<int, QByteArray> DccManager::roleNames() const {
  return {
    {IdRole, "transferId"},
    {NickRole, "nick"},
    {FileNameRole, "fileName"},
    {FileSizeRole, "fileSize"},
    {TransferredRole, "transferred"},
    {ProgressRole, "progress"},
    {StatusRole, "status"},
    {SpeedRole, "speed"},
    {EtaRole, "eta"},
    {DirectionRole, "direction"}
  };
}

QVariant DccManager::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_transfers.size()) return {};
  auto *t = m_transfers[index.row()];
  switch (role) {
    case IdRole: return t->id;
    case NickRole: return t->nick;
    case FileNameRole: return t->fileName;
    case FileSizeRole: return t->sizeString();
    case TransferredRole: return t->transferred;
    case ProgressRole: return t->progress();
    case StatusRole: return t->statusString();
    case SpeedRole: return t->speedString();
    case EtaRole: return t->etaString();
    case DirectionRole: return t->direction == DccTransfer::Send ? "Send" : "Recv";
  }
  return {};
}

void DccManager::refreshRow(DccTransfer *t) {
  int row = m_transfers.indexOf(t);
  if (row >= 0) {
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
  }
}

DccTransfer *DccManager::findTransfer(const QString &id) {
  for (auto *t : m_transfers)
    if (t->id == id) return t;
  return nullptr;
}

// ── Handle incoming DCC SEND offer ──
void DccManager::handleDccSend(const QString &nick, const QString &fileName,
                               quint32 ip, quint16 port, qint64 size, int token) {
  // Check file size limit
  if (m_maxFileSize > 0 && size > m_maxFileSize) {
    emit transferFailed(nick, fileName,
        "File exceeds maximum size limit (" + DccTransfer::formatSize(m_maxFileSize) + ")");
    return;
  }

  auto *t = new DccTransfer(this);
  t->nick = nick;
  t->fileName = fileName;
  t->fileSize = size;
  t->peerAddress = ip;
  t->peerPort = port;
  t->token = token;
  t->direction = DccTransfer::Receive;

  connect(t, &DccTransfer::stateChanged, this, [this, t]() {
    refreshRow(t);
    if (t->state == DccTransfer::Complete)
      emit transferComplete(t->nick, t->fileName);
    else if (t->state == DccTransfer::Failed)
      emit transferFailed(t->nick, t->fileName, t->errorString);
  });
  connect(t, &DccTransfer::progressUpdated, this, [this, t]() { refreshRow(t); });

  beginInsertRows({}, m_transfers.size(), m_transfers.size());
  m_transfers.append(t);
  endInsertRows();
  emit countChanged();
  emit transferAdded(nick, fileName, size);

  // Auto-accept if enabled
  if (m_autoAccept) {
    t->startReceive(m_downloadDir);
  }
}

void DccManager::acceptTransfer(const QString &id) {
  auto *t = findTransfer(id);
  if (!t || t->state != DccTransfer::Pending) return;

  // Ensure download dir exists
  QDir().mkpath(m_downloadDir);
  t->startReceive(m_downloadDir);
}

void DccManager::cancelTransfer(const QString &id) {
  auto *t = findTransfer(id);
  if (t) t->cancel();
}

void DccManager::clearFinished() {
  for (int i = m_transfers.size() - 1; i >= 0; --i) {
    auto *t = m_transfers[i];
    if (t->state == DccTransfer::Complete || t->state == DccTransfer::Failed ||
        t->state == DccTransfer::Cancelled) {
      beginRemoveRows({}, i, i);
      m_transfers.removeAt(i);
      endRemoveRows();
      t->deleteLater();
    }
  }
  emit countChanged();
}

void DccManager::sendFile(const QString &nick, const QString &filePath) {
  QFileInfo fi(filePath);
  if (!fi.exists() || !fi.isFile()) return;

  auto *t = new DccTransfer(this);
  t->nick = nick;
  t->fileName = fi.fileName();
  t->fileSize = fi.size();
  t->direction = DccTransfer::Send;

  connect(t, &DccTransfer::stateChanged, this, [this, t]() {
    refreshRow(t);
    if (t->state == DccTransfer::Complete)
      emit transferComplete(t->nick, t->fileName);
    else if (t->state == DccTransfer::Failed)
      emit transferFailed(t->nick, t->fileName, t->errorString);
  });
  connect(t, &DccTransfer::progressUpdated, this, [this, t]() { refreshRow(t); });

  beginInsertRows({}, m_transfers.size(), m_transfers.size());
  m_transfers.append(t);
  endInsertRows();
  emit countChanged();

  // Start listening on a random high port
  t->startSend(filePath, 0); // 0 = OS picks port

  // Send DCC SEND CTCP to the remote user
  if (m_connection && t->peerPort > 0) {
    // Get our external IP (TODO: use configured IP or UPnP)
    // For now use 0 (passive DCC — remote should connect back)
    quint32 myIp = 0;
    QString ctcp = "DCC SEND " + t->fileName + " " +
                   QString::number(myIp) + " " +
                   QString::number(t->peerPort) + " " +
                   QString::number(t->fileSize);
    m_connection->sendCtcp(nick, ctcp);
  }
}
