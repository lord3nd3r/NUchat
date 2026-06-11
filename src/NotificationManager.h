#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QSystemTrayIcon>

class QMenu;
class QAction;
class QWindow;
class Settings;

class NotificationManager : public QObject {
    Q_OBJECT
public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    /// Wire up settings for preference-aware notifications
    void setSettings(Settings *settings);

    /// Set the application window for show/hide toggle
    void setWindow(QWindow *window);

    /// Show a desktop notification balloon (respects user prefs)
    void notify(const QString &title, const QString &body,
                bool isHighlight = false, bool isPrivate = false);

    /// Update the tray icon to indicate unread messages
    void setUnreadState(bool hasUnread, bool hasHighlight);

    /// Reset tray icon to normal (called when user reads messages)
    Q_INVOKABLE void clearUnreadState();

    /// Play a notification sound (respects user prefs)
    void playSound(bool isHighlight, bool isPrivate);

    /// Whether the system supports tray icons
    static bool isAvailable() { return QSystemTrayIcon::isSystemTrayAvailable(); }

signals:
    void showWindowRequested();
    void quitRequested();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void toggleWindowVisibility();

private:
    void buildTrayIcon();

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_showHideAction = nullptr;
    QAction *m_quitAction = nullptr;
    QWindow *m_window = nullptr;
    Settings *m_settings = nullptr;
    QIcon m_normalIcon;
    QIcon m_unreadIcon;
    QIcon m_highlightIcon;

    // Rate limiting — at most one balloon/sound per interval to prevent
    // notification spam in busy channels
    static constexpr int kNotifyThrottleMs = 2000;
    QElapsedTimer m_notifyLimiter;
    QElapsedTimer m_soundLimiter;
};
