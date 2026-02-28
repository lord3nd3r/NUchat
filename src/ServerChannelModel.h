#pragma once

#include <QStandardItemModel>

class ServerChannelModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit ServerChannelModel(QObject *parent = nullptr);

    // convenience helpers
    void addServer(const QString &name);
    void addChannel(const QString &serverName, const QString &channelName);
    bool hasChannel(const QString &serverName, const QString &channelName) const;
    Q_INVOKABLE bool moveChannel(const QString &serverName, int fromIndex, int toIndex);
};
