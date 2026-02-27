#pragma once

#include <QObject>

class IRCClient : public QObject
{
    Q_OBJECT
public:
    explicit IRCClient(QObject *parent = nullptr);
    ~IRCClient();

    // TODO: connection methods
};
