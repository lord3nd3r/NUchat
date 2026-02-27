#include "NotificationManager.h"

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
{
}

void NotificationManager::notify(const QString &title, const QString &body)
{
    Q_UNUSED(title)
    Q_UNUSED(body)
    // stub: integrate with QSystemTrayIcon or native notifications
}
