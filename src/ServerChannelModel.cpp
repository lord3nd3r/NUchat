#include "ServerChannelModel.h"
#include <QStandardItem>

ServerChannelModel::ServerChannelModel(QObject *parent)
    : QStandardItemModel(parent) {
  setHorizontalHeaderLabels({"Servers/Channels"});
}

void ServerChannelModel::addServer(const QString &name) {
  // Don't add duplicate server entries (top-level rows)
  QList<QStandardItem *> existing = findItems(name);
  for (QStandardItem *it : existing) {
    if (it && it->parent() == nullptr)  // top-level server row
      return;
  }
  QStandardItem *item = new QStandardItem(name);
  appendRow(item);
}

void ServerChannelModel::addChannel(const QString &serverName,
                                    const QString &channelName) {
  QList<QStandardItem *> items = findItems(serverName);
  if (!items.isEmpty()) {
    QStandardItem *serverItem = items.first();
    // Don't add duplicates
    for (int i = 0; i < serverItem->rowCount(); i++) {
      if (serverItem->child(i)->text() == channelName)
        return;
    }
    serverItem->appendRow(new QStandardItem(channelName));
  }
}

bool ServerChannelModel::hasChannel(const QString &serverName,
                                    const QString &channelName) const {
  QList<QStandardItem *> items = findItems(serverName);
  if (!items.isEmpty()) {
    QStandardItem *serverItem = items.first();
    for (int i = 0; i < serverItem->rowCount(); i++) {
      if (serverItem->child(i)->text() == channelName)
        return true;
    }
  }
  return false;
}

void ServerChannelModel::removeChannel(const QString &serverName,
                                       const QString &channelName) {
  QList<QStandardItem *> items = findItems(serverName);
  if (!items.isEmpty()) {
    QStandardItem *serverItem = items.first();
    for (int i = 0; i < serverItem->rowCount(); i++) {
      if (serverItem->child(i)->text() == channelName) {
        serverItem->removeRow(i);
        return;
      }
    }
  }
}

void ServerChannelModel::removeServer(const QString &serverName) {
  while (true) {
    QList<QStandardItem *> items = findItems(serverName);
    if (items.isEmpty())
      break;
    // Only consider top-level server rows (parent == nullptr)
    bool removed = false;
    for (QStandardItem *it : items) {
      if (it && it->parent() == nullptr) {
        removeRow(it->row());
        removed = true;
        break;  // re-find after structural change
      }
    }
    if (!removed)
      break;
  }
}

bool ServerChannelModel::moveChannel(const QString &serverName, int fromIndex,
                                     int toIndex) {
  QList<QStandardItem *> items = findItems(serverName);
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
