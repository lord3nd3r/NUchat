#pragma once

#include <QObject>

class NotificationManager : public QObject
{
    Q_OBJECT
public:
    explicit NotificationManager(QObject *parent = nullptr);
    void notify(const QString &title, const QString &body);
};
