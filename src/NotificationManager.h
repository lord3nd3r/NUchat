#pragma once

#include <QObject>

class QSystemTrayIcon;

class NotificationManager : public QObject
{
    Q_OBJECT
public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    // Show a desktop notification. Falls back to qDebug if tray is unavailable.
    void notify(const QString &title, const QString &body);

    // Provide an existing tray icon to use for notifications (optional).
    void setTrayIcon(QSystemTrayIcon *tray);

private:
    QSystemTrayIcon *m_tray = nullptr;
    bool m_ownsTray = false;
};
