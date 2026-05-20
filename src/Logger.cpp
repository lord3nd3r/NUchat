#include "Logger.h"

Logger::Logger(QObject *parent) : QObject(parent) {}

QString Logger::logDir() const {
  return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
         QStringLiteral("/logs");
}

QString Logger::logFilePath(const QString &network,
                            const QString &channel) const {
  QString safeName = channel.toLower();
  safeName.replace('/', '_');
  safeName.replace('\\', '_');
  safeName.replace(':', '_');
  return logDir() + "/" + network.toLower() + "/" + safeName + ".log";
}

void Logger::log(const QString &network, const QString &channel,
                 const QString &type, const QString &message) {
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
                                                 int maxLines) const {
  QVector<LogEntry> result;
  QString filePath = logFilePath(network, channel);
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly))
    return result;

  qint64 fileSize = file.size();
  if (fileSize == 0) {
    file.close();
    return result;
  }

  // ── Seek from EOF: read backwards in chunks to find enough lines ──
  // We read 8 KB chunks from the end, accumulating raw bytes until we
  // have at least maxLines+1 newlines (to guarantee maxLines full lines).
  constexpr int kChunkSize = 8192;
  QByteArray tail;
  qint64 readPos = fileSize;
  int newlineCount = 0;
  int targetNewlines = maxLines + 1; // +1 because the last line may not end with \n

  while (readPos > 0 && newlineCount < targetNewlines) {
    qint64 chunkStart = qMax(qint64(0), readPos - kChunkSize);
    qint64 toRead = readPos - chunkStart;
    file.seek(chunkStart);
    QByteArray chunk = file.read(toRead);
    tail.prepend(chunk);
    readPos = chunkStart;
    // Count newlines in this chunk
    for (char c : chunk) {
      if (c == '\n') ++newlineCount;
    }
  }
  file.close();

  // Split the tail into lines and take the last maxLines
  QString text = QString::fromUtf8(tail);
  QStringList allLines = text.split('\n', Qt::SkipEmptyParts);

  int start = qMax(0, allLines.size() - maxLines);
  for (int i = start; i < allLines.size(); ++i) {
    const QString &line = allLines[i];
    if (line.length() < 20)
      continue; // skip malformed lines

    LogEntry entry;
    entry.timestamp = line.left(19); // "yyyy-MM-dd HH:mm:ss"

    // Parse type: new format has " [type] ", old format has no brackets
    int bracketOpen = line.indexOf('[', 20);
    int bracketClose = (bracketOpen >= 0) ? line.indexOf(']', bracketOpen) : -1;
    if (bracketOpen == 20 && bracketClose > bracketOpen) {
      // New format: "yyyy-MM-dd HH:mm:ss [chat] <nick> message"
      entry.type = line.mid(bracketOpen + 1, bracketClose - bracketOpen - 1);
      entry.text = line.mid(bracketClose + 2); // skip "] "
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
      if (t.startsWith("Now talking in ") || t.startsWith("Topic for ") ||
          t.startsWith("*** Topic for ") || t.contains("Scrollback from ") ||
          t.contains("End of scrollback") || t.startsWith("Connecting to "))
        continue;
    }
    result.append(entry);
  }
  return result;
}
