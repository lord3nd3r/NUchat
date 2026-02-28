#include "Logger.h"

Logger::Logger(QObject *parent)
    : QObject(parent)
{
}

QString Logger::logDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/logs");
}

void Logger::log(const QString &network, const QString &channel, const QString &message)
{
    // Ensure log directory exists: ~/.config/NUchat/logs/<network>/
    QString dir = logDir() + "/" + network;
    QDir().mkpath(dir);

    // Sanitize channel name for filename (replace / and other bad chars)
    QString safeName = channel;
    safeName.replace('/', '_');
    safeName.replace('\\', '_');
    safeName.replace(':', '_');

    QString filePath = dir + "/" + safeName + ".log";
    QFile file(filePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        stream << ts << " " << message << "\n";
    }
}
