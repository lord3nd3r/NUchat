#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QVector>

class Settings;

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    void setSettings(Settings *settings) { m_settings = settings; }
    void log(const QString &network, const QString &channel,
             const QString &type, const QString &message);
    QString logDir() const;

    struct LogEntry {
        QString timestamp;
        QString type;
        QString text;
    };
    QVector<LogEntry> loadScrollback(const QString &network, const QString &channel,
                                      int maxLines = 200) const;

private:
    QString logFilePath(const QString &network, const QString &channel) const;
    bool loggingEnabled() const;
    bool logPrivateMessages() const;
    bool perChannelLogs() const;
    bool writeTimestamps() const;

    Settings *m_settings = nullptr;
};
