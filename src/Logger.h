#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QVector>
#include <QSet>
#include <QMutex>

class QTimer;

class Settings;

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    ~Logger() override;
    void setSettings(Settings *settings) { m_settings = settings; }
    void log(const QString &network, const QString &channel,
             const QString &type, const QString &message);
    QString logDir() const;

    // Log lines are written to the OS (survives a process crash) immediately,
    // but fsync'd to disk in batches (every ~2 s, on quit and on destruction)
    // instead of once per line on the GUI thread.
    void flushPending();

    // Tests (and callers that want a private log root) override the default
    // AppConfigLocation/logs directory. Rotation keeps `maxBackups` numbered
    // files (.log.1 …) and starts a new file after `maxBytes`.
    void setLogDirectory(const QString &dir);
    void setRotationLimits(qint64 maxBytes, int maxBackups);

    struct LogEntry {
        QString timestamp;
        QString type;
        QString text;
    };
    QVector<LogEntry> loadScrollback(const QString &network, const QString &channel,
                                      int maxLines = 200) const;

private:
    QString logFilePath(const QString &network, const QString &channel) const;
    void rotateIfNeeded(const QString &filePath) const;
    bool loggingEnabled() const;
    bool logPrivateMessages() const;
    bool perChannelLogs() const;
    bool writeTimestamps() const;

    void markDirty(const QString &filePath);

    Settings *m_settings = nullptr;
    QMutex m_dirtyMutex;
    QSet<QString> m_dirty;
    QTimer *m_syncTimer = nullptr;
    QString m_logDirOverride;
    qint64 m_maxBytes = 10 * 1024 * 1024;
    int m_maxBackups = 5;
};
