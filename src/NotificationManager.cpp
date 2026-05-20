#include "NotificationManager.h"
#include "Settings.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QPainter>
#include <QProcess>
#include <QWindow>

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
{
    buildTrayIcon();
}

NotificationManager::~NotificationManager() {
    delete m_trayMenu;  // QMenu isn't parented to QObject tree when used as context menu
}

void NotificationManager::setSettings(Settings *settings) {
    m_settings = settings;
}

void NotificationManager::setWindow(QWindow *window) {
    m_window = window;
}

void NotificationManager::buildTrayIcon() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qDebug() << "[NotificationManager] System tray not available";
        return;
    }

    // Load normal icon from app resources
    m_normalIcon = QIcon(":/icons/nuchat.svg");
    if (m_normalIcon.isNull())
        m_normalIcon = QApplication::windowIcon();

    // Generate unread icon: normal icon with a blue dot overlay
    {
        QPixmap px = m_normalIcon.pixmap(64, 64);
        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(0x42, 0xa5, 0xf5)); // blue
        p.setPen(Qt::NoPen);
        p.drawEllipse(px.width() - 18, 0, 16, 16);
        m_unreadIcon = QIcon(px);
    }

    // Generate highlight icon: normal icon with a red dot overlay
    {
        QPixmap px = m_normalIcon.pixmap(64, 64);
        QPainter p(&px);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(0xf4, 0x47, 0x47)); // red
        p.setPen(Qt::NoPen);
        p.drawEllipse(px.width() - 18, 0, 16, 16);
        m_highlightIcon = QIcon(px);
    }

    // Build context menu
    m_trayMenu = new QMenu();
    m_showHideAction = m_trayMenu->addAction("Show/Hide NUchat");
    connect(m_showHideAction, &QAction::triggered, this, &NotificationManager::toggleWindowVisibility);

    m_trayMenu->addSeparator();
    m_quitAction = m_trayMenu->addAction("Quit");
    connect(m_quitAction, &QAction::triggered, this, &NotificationManager::quitRequested);

    // Create and show the tray icon
    m_tray = new QSystemTrayIcon(m_normalIcon, this);
    m_tray->setToolTip("NUchat");
    m_tray->setContextMenu(m_trayMenu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &NotificationManager::onTrayActivated);

    m_tray->show();
    qDebug() << "[NotificationManager] System tray icon created";
}

void NotificationManager::notify(const QString &title, const QString &body,
                                  bool isHighlight, bool isPrivate) {
    // Check user preferences before showing notification
    if (m_settings) {
        if (isHighlight && !m_settings->getBool("notify/highlight", true))
            return;
        if (isPrivate && !m_settings->getBool("notify/privateMessage", true))
            return;
    }

    // Don't notify if the window is focused (user is looking at it)
    if (m_window && m_window->isActive())
        return;

    if (m_tray && QSystemTrayIcon::supportsMessages()) {
        int timeout = m_settings ? m_settings->getInt("notify/timeout", 5000) : 5000;
        m_tray->showMessage(title, body, QSystemTrayIcon::Information, timeout);
    } else {
        qDebug() << "[Notification]" << title << "-" << body;
    }
}

void NotificationManager::setUnreadState(bool hasUnread, bool hasHighlight) {
    if (!m_tray) return;

    if (hasHighlight)
        m_tray->setIcon(m_highlightIcon);
    else if (hasUnread)
        m_tray->setIcon(m_unreadIcon);
    else
        m_tray->setIcon(m_normalIcon);
}

void NotificationManager::clearUnreadState() {
    setUnreadState(false, false);
}

void NotificationManager::playSound(bool isHighlight, bool isPrivate) {
    // Check if sounds are enabled at all
    if (m_settings && !m_settings->getBool("sound/enable", true))
        return;

    // Check per-event sound preferences
    if (m_settings) {
        if (isHighlight && !m_settings->getBool("sound/highlight", true))
            return;
        if (isPrivate && !m_settings->getBool("sound/privateMessage", true))
            return;
    }

    // Use configured beep command, or fall back to system bell
    QString beepCmd = m_settings ? m_settings->getString("sound/beepCommand") : QString();
    if (beepCmd.isEmpty()) {
        QApplication::beep();
    } else {
        QProcess::startDetached("/bin/sh", {"-c", beepCmd});
    }
}

void NotificationManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger ||
        reason == QSystemTrayIcon::DoubleClick) {
        toggleWindowVisibility();
    }
}

void NotificationManager::toggleWindowVisibility() {
    if (!m_window) {
        emit showWindowRequested();
        return;
    }

    if (m_window->isVisible()) {
        m_window->hide();
    } else {
        m_window->show();
        m_window->raise();
        m_window->requestActivate();
    }
}
