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

QString Logger::logFilePath(const QString &network, const QString &channel) const
{
    QString safeName = channel;
    safeName.replace('/', '_');
    safeName.replace('\\', '_');
    safeName.replace(':', '_');
    return logDir() + "/" + network + "/" + safeName + ".log";
}

void Logger::log(const QString &network, const QString &channel,
                 const QString &type, const QString &message)
{
    // Ensure log directory exists: ~/.config/NUchat/logs/<network>/
    QString dir = logDir() + "/" + network;
    QDir().mkpath(dir);

    QString filePath = logFilePath(network, channel);
    QFile file(filePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        stream << ts << " [" << type << "] " << message << "\n";
    }
}

QVector<Logger::LogEntry> Logger::loadScrollback(const QString &network,
                                                   const QString &channel,
                                                   int maxLines) const
{
    QVector<LogEntry> result;
    QString filePath = logFilePath(network, channel);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    // Read all lines (could be optimised for huge files, but fine for IRC logs)
    QStringList allLines;
    QTextStream in(&file);
    while (!in.atEnd())
        allLines.append(in.readLine());
    file.close();

    // Take last maxLines
    int start = qMax(0, allLines.size() - maxLines);
    for (int i = start; i < allLines.size(); ++i) {
        const QString &line = allLines[i];
        if (line.length() < 20) continue;  // skip malformed lines

        LogEntry entry;
        entry.timestamp = line.left(19);  // "yyyy-MM-dd HH:mm:ss"

        // Parse type: new format has " [type] ", old format has no brackets
        int bracketOpen = line.indexOf('[', 20);
        int bracketClose = (bracketOpen >= 0) ? line.indexOf(']', bracketOpen) : -1;
        if (bracketOpen == 20 && bracketClose > bracketOpen) {
            // New format: "yyyy-MM-dd HH:mm:ss [chat] <nick> message"
            entry.type = line.mid(bracketOpen + 1, bracketClose - bracketOpen - 1);
            entry.text = line.mid(bracketClose + 2);  // skip "] "
        } else {
            // Old format: "yyyy-MM-dd HH:mm:ss <nick> message" — guess type
            QString rest = line.mid(20);
            if (rest.startsWith('<') || rest.startsWith('-'))
                entry.type = QStringLiteral("chat");
            else if (rest.startsWith('*'))
                entry.type = QStringLiteral("action");
            else
                entry.type = QStringLiteral("system");
            entry.text = rest;
        }
        // Filter out noisy system messages from scrollback
        if (entry.type == "system") {
            const QString &t = entry.text;
            if (t.startsWith("Now talking in ") ||
                t.startsWith("Topic for ") ||
                t.startsWith("*** Topic for ") ||
                t.contains("Scrollback from ") ||
                t.contains("End of scrollback") ||
                t.startsWith("Connecting to "))
                continue;
        }
        result.append(entry);
    }
    return result;
}
