#include "DccManager.h"
#include "IrcConnection.h"
#include "NetSafety.h"
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QSettings>
#include <QUuid>
#include <QtEndian>

// ════════════════════════════════════════════════════════
//  DccTransfer
// ════════════════════════════════════════════════════════

DccTransfer::DccTransfer(QObject *parent) : QObject(parent) {
  id = QUuid::createUuid().toString(QUuid::Id128).left(8);
  m_speedTimer.setInterval(1000);
  connect(&m_speedTimer, &QTimer::timeout, this, &DccTransfer::updateSpeed);
  m_idleTimer.setSingleShot(true);
  connect(&m_idleTimer, &QTimer::timeout, this, &DccTransfer::onIdleTimeout);
}

DccTransfer::~DccTransfer() {
  m_idleTimer.stop();
  m_speedTimer.stop();
  if (m_file.isOpen()) m_file.close();
  // A receive destroyed mid-transfer should not leave a partial file behind.
  if (direction == Receive && state != Complete && !filePath.isEmpty())
    QFile::remove(filePath);
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

void DccTransfer::armIdleTimer(int ms) { m_idleTimer.start(ms); }

void DccTransfer::failTransfer(const QString &reason) {
  if (state == Complete || state == Cancelled || state == Failed)
    return;
  state = Failed;
  errorString = reason;
  m_speedTimer.stop();
  m_idleTimer.stop();
  if (m_socket) m_socket->abort();
  if (m_server) m_server->close();
  if (m_file.isOpen()) m_file.close();
  // Only downloads are ours to delete. A send uses the user's original file.
  if (direction == Receive && !filePath.isEmpty())
    QFile::remove(filePath);
  emit stateChanged();
}

void DccTransfer::onIdleTimeout() {
  if (state == Connecting)
    failTransfer(QStringLiteral("Connection timed out"));
  else if (state == Pending && direction == Send)
    failTransfer(QStringLiteral("Timed out waiting for the peer to connect"));
  else if (state == Transferring)
    failTransfer(QStringLiteral("Transfer stalled"));
}

// ── Receive ──
void DccTransfer::startReceive(const QString &downloadDir) {
  if (state != Pending) return;

  // Manual accept is the opt-in for a private-LAN peer. Loopback and
  // non-unicast addresses are still refused here.
  const QString blocked = NetSafety::dccReceiveBlockReason(peerAddress, peerPort, true);
  if (!blocked.isEmpty()) {
    failTransfer(blocked);
    return;
  }

  const NetSafety::ProxyResolution proxy = NetSafety::proxyFromSettings();
  if (proxy.result == NetSafety::ProxyResolution::Result::Rejected) {
    failTransfer(proxy.error);
    return;
  }

  const QString safeName = NetSafety::sanitizeDccFileName(fileName);
  QDir dir(downloadDir);
  dir.mkpath(QStringLiteral("."));
  filePath = dir.absoluteFilePath(safeName);

  // Avoid overwriting: append (1), (2), etc.
  if (QFile::exists(filePath)) {
    QString base = QFileInfo(filePath).completeBaseName();
    QString ext = QFileInfo(filePath).suffix();
    int n = 1;
    while (QFile::exists(filePath)) {
      filePath = dir.absoluteFilePath(base + "(" + QString::number(n++) + ")" +
                                       (ext.isEmpty() ? "" : "." + ext));
    }
  }

  m_file.setFileName(filePath);
  if (!m_file.open(QIODevice::WriteOnly)) {
    state = Failed;
    errorString = "Cannot open file for writing: " + m_file.errorString();
    filePath.clear();
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

  if (proxy.result == NetSafety::ProxyResolution::Result::UseProxy)
    m_socket->setProxy(proxy.proxy);
  else
    m_socket->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));

  QHostAddress addr(peerAddress);
  m_socket->connectToHost(addr, peerPort);
  armIdleTimer(kConnectTimeoutMs);
}

void DccTransfer::onConnected() {
  state = Transferring;
  m_elapsed.start();
  m_lastBytes = 0;
  m_speedTimer.start();
  armIdleTimer(kIdleTimeoutMs);
  emit stateChanged();
}

void DccTransfer::onDataReady() {
  armIdleTimer(kIdleTimeoutMs);
  while (m_socket && m_socket->bytesAvailable() > 0) {
    QByteArray data = m_socket->read(CHUNK_SIZE);
    const qint64 wrote = m_file.write(data);
    if (wrote != data.size()) {
      failTransfer(QStringLiteral("Failed to write to disk"));
      return;
    }
    transferred += data.size();
    // A peer sending more than it offered is misbehaving — stop before it
    // can fill the disk (allow one chunk of slack for the final read)
    if (fileSize > 0 && transferred > fileSize + CHUNK_SIZE) {
      failTransfer(QStringLiteral("Peer sent more data than the offered file size"));
      return;
    }
  }

  // Send DCC acknowledgment (4-byte big-endian total received)
  sendAck();

  if (fileSize > 0 && transferred >= fileSize) {
    state = Complete;
    m_speedTimer.stop();
    m_idleTimer.stop();
    m_file.close();
    if (m_socket) m_socket->disconnectFromHost();
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
void DccTransfer::startSend(const QString &localPath, quint16 listenPort,
                           const QHostAddress &bindAddr) {
  direction = Send;
  // Active DCC listens for a direct inbound connection and advertises our
  // address. That bypasses a configured proxy, so refuse rather than leak.
  const NetSafety::ProxyResolution proxy = NetSafety::proxyFromSettings();
  if (proxy.result != NetSafety::ProxyResolution::Result::Direct) {
    state = Failed;
    errorString = proxy.result == NetSafety::ProxyResolution::Result::Rejected
                      ? proxy.error
                      : QStringLiteral(
                            "Active DCC send opens a direct listening socket and would "
                            "reveal your IP while a proxy is configured");
    emit stateChanged();
    return;
  }
  if (!NetSafety::isUsableDccBindAddress(bindAddr)) {
    state = Failed;
    errorString = QStringLiteral(
        "Refusing to listen on all interfaces; no IPv4 address is available to advertise");
    emit stateChanged();
    return;
  }

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

  if (!m_server->listen(bindAddr, listenPort)) {
    state = Failed;
    errorString = "Cannot listen on " + bindAddr.toString() + ":" +
                  QString::number(listenPort) + ": " + m_server->errorString();
    m_file.close();
    emit stateChanged();
    return;
  }

  peerPort = m_server->serverPort();
  state = Pending; // waiting for remote to connect
  armIdleTimer(kAcceptTimeoutMs);
  emit stateChanged();
}

void DccTransfer::onNewConnection() {
  if (!m_server)
    return;
  // One peer only. Anyone else who races the port is dropped.
  m_socket = m_server->nextPendingConnection();
  m_server->close();
  if (!m_socket)
    return;
  if (QTcpSocket *extra = m_server->nextPendingConnection()) {
    extra->abort();
    extra->deleteLater();
  }

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
  armIdleTimer(kIdleTimeoutMs);
  emit stateChanged();
  sendNextChunk();
}

void DccTransfer::sendNextChunk() {
  if (!m_socket || !m_file.isOpen() || state != Transferring) return;
  // Don't flood the socket buffer
  if (m_socket->bytesToWrite() > CHUNK_SIZE * 4) return;

  QByteArray data = m_file.read(CHUNK_SIZE);
  if (data.isEmpty()) return; // EOF — wait for ack

  armIdleTimer(kIdleTimeoutMs);
  m_socket->write(data);
  transferred += data.size();
  emit progressUpdated();

  if (transferred >= fileSize) {
    // All data sent, wait for remote to close or ack
  }
}

void DccTransfer::onSendDataReady() {
  armIdleTimer(kIdleTimeoutMs);
  // Read acknowledgment from receiver (4-byte big-endian)
  while (m_socket && m_socket->bytesAvailable() >= 4) {
    char ackBuf[4];
    m_socket->read(ackBuf, 4);
    quint32 acked = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(ackBuf));
    if (acked >= static_cast<quint32>(fileSize & 0xFFFFFFFF) && transferred >= fileSize) {
      state = Complete;
      m_speedTimer.stop();
      m_idleTimer.stop();
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
  m_idleTimer.stop();
  if (m_socket) m_socket->abort();
  if (m_server) m_server->close();
  if (m_file.isOpen())
    m_file.close();
  if (direction == Receive && !filePath.isEmpty())
    QFile::remove(filePath);
  emit stateChanged();
}

void DccTransfer::onDisconnected() {
  if (state != Transferring)
    return;
  m_speedTimer.stop();
  m_idleTimer.stop();
  if (fileSize > 0 && transferred >= fileSize) {
    state = Complete;
    if (m_file.isOpen()) m_file.close();
    emit stateChanged();
    return;
  }
  failTransfer(QStringLiteral("Connection closed prematurely"));
}

void DccTransfer::onError(QAbstractSocket::SocketError) {
  if (state == Complete || state == Cancelled || state == Failed)
    return;
  const QString why = m_socket ? m_socket->errorString() : QStringLiteral("Socket error");
  failTransfer(why.isEmpty() ? QStringLiteral("Socket error") : why);
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

int DccManager::activeTransferCount() const {
  int n = 0;
  for (auto *t : m_transfers) {
    if (t->state == DccTransfer::Pending || t->state == DccTransfer::Connecting ||
        t->state == DccTransfer::Transferring)
      ++n;
  }
  return n;
}

// ── Handle incoming DCC SEND offer ──
void DccManager::handleDccSend(const QString &nick, const QString &fileName,
                               quint32 ip, quint16 port, qint64 size, int token) {
  // Reject offers without a positive size — the receive loop uses the
  // declared size as its only termination condition, so accepting a
  // zero/negative size would let the peer stream data (and fill the disk)
  // indefinitely.
  if (size <= 0) {
    emit transferFailed(nick, fileName, "Invalid file size in DCC offer");
    return;
  }
  // Check file size limit
  if (m_maxFileSize > 0 && size > m_maxFileSize) {
    emit transferFailed(nick, fileName,
        "File exceeds maximum size limit (" + DccTransfer::formatSize(m_maxFileSize) + ")");
    return;
  }
  // Loopback, 0.0.0.0, link-local, and other non-unicast targets are a
  // connect-back / port-scan primitive. Private LAN addresses are kept as a
  // pending offer so the user can accept them, but they are not auto-connected.
  const QString blocked = NetSafety::dccReceiveBlockReason(ip, port, true);
  if (!blocked.isEmpty()) {
    emit transferFailed(nick, fileName, blocked);
    return;
  }
  if (activeTransferCount() >= kMaxActiveTransfers) {
    emit transferFailed(nick, fileName, "Too many DCC transfers already in progress");
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

  // Auto-accept only public peers unless the user opted in via
  // dcc/allowPrivatePeers. A manual accept still allows a private-LAN peer.
  if (m_autoAccept) {
    const bool allowPrivate =
        QSettings().value(QStringLiteral("dcc/allowPrivatePeers"), false).toBool();
    if (NetSafety::dccReceiveBlockReason(ip, port, allowPrivate).isEmpty())
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

  // Bind to the IRC socket's own IPv4 address. Listening on Any, or
  // advertising 0 for an IPv6 local address, would expose every interface
  // or tell the peer to connect to 0.0.0.0.
  QHostAddress bindAddr;
  quint32 myIp = 0;
  if (m_connection) {
    const QHostAddress local = m_connection->localAddress();
    if (local.protocol() == QAbstractSocket::IPv4Protocol) {
      bindAddr = local;
      myIp = local.toIPv4Address();
    } else if (local.protocol() == QAbstractSocket::IPv6Protocol) {
      bool ok = false;
      const quint32 v4 = local.toIPv4Address(&ok);
      if (ok && v4 != 0) {
        bindAddr = QHostAddress(v4);
        myIp = v4;
      }
    }
  }
  t->startSend(filePath, 0, bindAddr); // 0 = OS picks port

  // Send DCC SEND CTCP to the remote user. Skip when listen failed or we
  // have nothing but 0.0.0.0 to advertise.
  if (m_connection && t->state != DccTransfer::Failed && t->peerPort > 0 && myIp != 0) {
    QString offerName = t->fileName.contains(' ')
        ? "\"" + t->fileName + "\"" : t->fileName;
    QString ctcp = "DCC SEND " + offerName + " " +
                   QString::number(myIp) + " " +
                   QString::number(t->peerPort) + " " +
                   QString::number(t->fileSize);
    m_connection->sendCtcp(nick, ctcp);
  }
}
