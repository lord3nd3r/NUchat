#include "ServerChannelModel.h"
#include "IrcIdentity.h"
#include <QStandardItem>

ServerChannelModel::ServerChannelModel(QObject *parent)
    : QStandardItemModel(parent) {
  setHorizontalHeaderLabels({"Servers/Channels"});
}

static QList<QStandardItem *> findTopLevel(QStandardItemModel *model,
                                           const QString &name) {
  // MatchFixedString is case-insensitive unless MatchCaseSensitive is set.
  QList<QStandardItem *> found = model->findItems(name, Qt::MatchFixedString);
  QList<QStandardItem *> top;
  for (QStandardItem *it : found) {
    if (it && it->parent() == nullptr)
      top.append(it);
  }
  return top;
}

void ServerChannelModel::addServer(const QString &name) {
  if (!findTopLevel(this, name).isEmpty())
    return;
  QStandardItem *item = new QStandardItem(name);
  appendRow(item);
}

void ServerChannelModel::addChannel(const QString &serverName,
                                    const QString &channelName) {
  const QList<QStandardItem *> items = findTopLevel(this, serverName);
  if (!items.isEmpty()) {
    QStandardItem *serverItem = items.first();
    for (int i = 0; i < serverItem->rowCount(); i++) {
      if (ircSame(serverItem->child(i)->text(), channelName))
        return;
    }
    serverItem->appendRow(new QStandardItem(channelName));
  }
}

bool ServerChannelModel::hasChannel(const QString &serverName,
                                    const QString &channelName) const {
  const QList<QStandardItem *> items =
      findTopLevel(const_cast<ServerChannelModel *>(this), serverName);
  if (!items.isEmpty()) {
    QStandardItem *serverItem = items.first();
    for (int i = 0; i < serverItem->rowCount(); i++) {
      if (ircSame(serverItem->child(i)->text(), channelName))
        return true;
    }
  }
  return false;
}

void ServerChannelModel::removeChannel(const QString &serverName,
                                       const QString &channelName) {
  const QList<QStandardItem *> items = findTopLevel(this, serverName);
  if (!items.isEmpty()) {
    QStandardItem *serverItem = items.first();
    for (int i = 0; i < serverItem->rowCount(); i++) {
      if (ircSame(serverItem->child(i)->text(), channelName)) {
        serverItem->removeRow(i);
        return;
      }
    }
  }
}

void ServerChannelModel::removeServer(const QString &serverName) {
  while (true) {
    QList<QStandardItem *> items = findTopLevel(this, serverName);
    if (items.isEmpty())
      break;
    removeRow(items.first()->row());
  }
}

bool ServerChannelModel::moveChannel(const QString &serverName, int fromIndex,
                                     int toIndex) {
  QList<QStandardItem *> items = findTopLevel(this, serverName);
  if (items.isEmpty())
    return false;
  QStandardItem *serverItem = items.first();
  int count = serverItem->rowCount();
  if (fromIndex < 0 || fromIndex >= count || toIndex < 0 || toIndex >= count ||
      fromIndex == toIndex)
    return false;
  QList<QStandardItem *> taken = serverItem->takeRow(fromIndex);
  if (taken.isEmpty())
    return false;
  serverItem->insertRow(toIndex, taken);
  return true;
}
