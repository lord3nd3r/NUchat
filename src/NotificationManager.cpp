#include "NotificationManager.h"

#include <QSystemTrayIcon>
#include <QApplication>
#include <QDebug>

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
{
    // Create an internal tray icon only if the system supports it and one
    // hasn't been supplied via setTrayIcon().
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_tray = new QSystemTrayIcon(QApplication::windowIcon(), this);
        m_tray->show();
        m_ownsTray = true;
    }
}

NotificationManager::~NotificationManager()
{
    if (m_ownsTray && m_tray)
        m_tray->hide();
}

void NotificationManager::setTrayIcon(QSystemTrayIcon *tray)
{
    if (m_ownsTray && m_tray) {
        m_tray->hide();
        m_tray->deleteLater();
    }
    m_tray = tray;
    m_ownsTray = false;
}

void NotificationManager::notify(const QString &title, const QString &body)
{
    if (m_tray && QSystemTrayIcon::isSystemTrayAvailable() &&
        QSystemTrayIcon::supportsMessages()) {
        m_tray->showMessage(title, body, QSystemTrayIcon::Information,
                            /*msecs=*/5000);
    } else {
        qDebug() << "[Notification]" << title << "-" << body;
    }
}
