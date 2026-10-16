#include "Logger.h"
#include "IrcIdentity.h"
#include "Settings.h"
#include "UserPaths.h"

#include <QFileDevice>
#include <QFileInfo>
#include <QRegularExpression>

Logger::Logger(QObject *parent) : QObject(parent) {}

bool Logger::loggingEnabled() const {
  if (!m_settings)
    return true;
  return m_settings->getBool(QStringLiteral("log/enable"), true);
}

bool Logger::logPrivateMessages() const {
  if (!m_settings)
    return true;
  return m_settings->getBool(QStringLiteral("log/privateMessages"), true);
}

bool Logger::perChannelLogs() const {
  if (!m_settings)
    return true;
  return m_settings->getBool(QStringLiteral("log/perChannel"), true);
}

bool Logger::writeTimestamps() const {
  if (!m_settings)
    return true;
  return m_settings->getBool(QStringLiteral("log/timestamps"), true);
}

QString Logger::logDir() const {
  QString dir;
  if (m_settings)
    dir = m_settings->getString(QStringLiteral("log/directory"));
  if (dir.isEmpty()) {
    dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
          QStringLiteral("/logs");
  } else {
    dir = expandHomePath(dir);
  }
  return QDir::cleanPath(dir);
}

// Replace characters that are invalid in filenames (Windows is the strictest:
// \ / : * ? " < > |) so channels like #foo|bar still get a log file.
// A component of "." or ".." would walk out of the log directory.
static QString sanitizeFileName(QString name) {
  name = name.trimmed();
  QString out;
  out.reserve(name.size());
  for (const QChar &c : name) {
    const ushort u = c.unicode();
    if (u < 32 || u == 127 || QStringLiteral("/\\:*?\"<>|").contains(c))
      out += QLatin1Char('_');
    else
      out += c;
  }
  if (out.isEmpty() || out == QLatin1String(".") || out == QLatin1String(".."))
    out = QLatin1Char('_') + out;
  return out;
}

static void restrictToOwner(const QString &path, bool isDir) {
  const QFileDevice::Permissions perms =
      isDir ? (QFileDevice::ReadOwner | QFileDevice::WriteOwner |
               QFileDevice::ExeOwner)
            : (QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  QFile::setPermissions(path, perms);
}

QString Logger::logFilePath(const QString &network,
                            const QString &channel) const {
  const QString channelPart =
      perChannelLogs() ? channel : QStringLiteral("messages");
  return logDir() + "/" + sanitizeFileName(network.toLower()) + "/" +
         sanitizeFileName(channelPart.toLower()) + ".log";
}

void Logger::log(const QString &network, const QString &channel,
                 const QString &type, const QString &message) {
  if (!loggingEnabled() || network.isEmpty() || channel.isEmpty())
    return;

  const bool isChannel =
      channel.startsWith(QLatin1Char('#')) || channel.startsWith(QLatin1Char('&'));
  const bool isServerTab = ircSame(channel, network);
  if (!isChannel && !isServerTab && !logPrivateMessages())
    return;

  const QString root = logDir();
  const bool rootExisted = QFileInfo::exists(root);
  QDir().mkpath(root);
  if (!rootExisted)
    restrictToOwner(root, true);

  const QString dir = root + "/" + sanitizeFileName(network.toLower());
  const bool dirExisted = QFileInfo::exists(dir);
  QDir().mkpath(dir);
  if (!dirExisted)
    restrictToOwner(dir, true);

  QString filePath = logFilePath(network, channel);

  // ── Size-based rotation: keep one .log.1 backup ──
  static constexpr qint64 kMaxLogSize = 10 * 1024 * 1024; // 10 MB
  if (QFileInfo(filePath).size() > kMaxLogSize) {
    QString backup = filePath + ".1";
    QFile::remove(backup);
    QFile::rename(filePath, backup);
  }

  QFile file(filePath);
  if (file.open(QIODevice::Append | QIODevice::Text)) {
    restrictToOwner(filePath, false);
    QTextStream stream(&file);
    if (writeTimestamps()) {
      QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
      stream << ts << " [" << type << "] " << message << "\n";
    } else {
      stream << "[" << type << "] " << message << "\n";
    }
    stream.flush();
  }
}

static bool hasTimestampPrefix(const QString &line) {
  static const QRegularExpression re(
      QStringLiteral("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}"));
  return re.match(line).hasMatch();
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
    LogEntry entry;

    if (hasTimestampPrefix(line)) {
      if (line.length() < 20)
        continue;
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
    } else if (line.startsWith(QLatin1Char('['))) {
      // Timestamps disabled: "[type] message"
      const int bracketClose = line.indexOf(QLatin1Char(']'));
      if (bracketClose <= 1)
        continue;
      entry.type = line.mid(1, bracketClose - 1);
      entry.text = line.mid(bracketClose + 1).trimmed();
    } else {
      continue;
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
