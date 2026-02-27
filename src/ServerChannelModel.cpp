#include "ServerChannelModel.h"
#include <QStandardItem>

ServerChannelModel::ServerChannelModel(QObject *parent)
    : QStandardItemModel(parent)
{
    setHorizontalHeaderLabels({"Servers/Channels"});
}

void ServerChannelModel::addServer(const QString &name)
{
    QStandardItem *item = new QStandardItem(name);
    appendRow(item);
}

void ServerChannelModel::addChannel(const QString &serverName, const QString &channelName)
{
    QList<QStandardItem*> items = findItems(serverName);
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

bool ServerChannelModel::hasChannel(const QString &serverName, const QString &channelName) const
{
    QList<QStandardItem*> items = findItems(serverName);
    if (!items.isEmpty()) {
        QStandardItem *serverItem = items.first();
        for (int i = 0; i < serverItem->rowCount(); i++) {
            if (serverItem->child(i)->text() == channelName)
                return true;
        }
    }
    return false;
}
