#include "Logger.h"
#include "IrcIdentity.h"
#include "Settings.h"
#include "UserPaths.h"

#include <QDebug>
#include <QFileDevice>
#include <QFileInfo>
#include <QRegularExpression>

#if defined(Q_OS_UNIX)
#include <sys/stat.h>
#include <unistd.h>
#endif
#if defined(Q_OS_WIN)
#include <io.h>
#include <windows.h>
#endif

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
  // An explicit override (tests, callers that want a private root) wins over
  // the Preferences directory.
  if (!m_logDirOverride.isEmpty())
    return QDir::cleanPath(m_logDirOverride);
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

void Logger::setLogDirectory(const QString &dir) { m_logDirOverride = dir; }

void Logger::setRotationLimits(qint64 maxBytes, int maxBackups) {
  m_maxBytes = maxBytes > 0 ? maxBytes : m_maxBytes;
  m_maxBackups = maxBackups > 0 ? maxBackups : 1;
}

// Replace characters that are invalid in filenames (Windows is the strictest:
// \ / : * ? " < > |) so channels like #foo|bar still get a log file.
// A component of "." or ".." would walk out of the log directory, so those
// names are prefixed with '_' (".." -> "_.."). Windows then strips trailing
// dots and spaces, which would store "_.." as "_", so a trailing '_' is
// added until the name no longer ends in either (".." -> "_.._").
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
  while (out.endsWith(QLatin1Char('.')) || out.endsWith(QLatin1Char(' ')))
    out += QLatin1Char('_');
  return out;
}

// Drop C0 controls, ANSI CSI sequences, and mIRC formatting so a log line
// stays one physical line and `cat` cannot be driven by escape sequences.
// Tabs and newlines become spaces so the words are still readable.
static QString sanitizeLogText(const QString &in) {
  QString out;
  out.reserve(in.size());
  for (int i = 0; i < in.size(); ++i) {
    const QChar c = in.at(i);
    const ushort u = c.unicode();

    if (u == 0x03) { // mIRC color: optional ff[,bb]
      int j = i + 1;
      int n = 0;
      while (n < 2 && j < in.size() && in.at(j).isDigit()) {
        ++j;
        ++n;
      }
      if (j < in.size() && in.at(j) == QLatin1Char(',')) {
        int k = j + 1;
        int m = 0;
        while (m < 2 && k < in.size() && in.at(k).isDigit()) {
          ++k;
          ++m;
        }
        if (m > 0)
          j = k;
      }
      // j == i+1 means a lone ^C. Leave i alone so the for-loop steps past it.
      if (j > i + 1)
        i = j - 1;
      continue;
    }
    if (u == 0x04) { // mIRC hex color: RRGGBB[,RRGGBB]
      auto hexRun = [&](int from) {
        int k = from;
        int m = 0;
        while (m < 6 && k < in.size()) {
          const ushort h = in.at(k).unicode();
          const bool hex = (h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') ||
                           (h >= 'A' && h <= 'F');
          if (!hex)
            break;
          ++k;
          ++m;
        }
        return m == 6 ? k : from;
      };
      int j = hexRun(i + 1);
      if (j != i + 1) {
        if (j < in.size() && in.at(j) == QLatin1Char(',')) {
          int k = hexRun(j + 1);
          if (k != j + 1)
            j = k;
        }
        i = j - 1;
        continue;
      }
    }
    // bold, reset, reverse, monospace, italic, strikethrough, underline, CTCP
    if (u == 0x02 || u == 0x0F || u == 0x16 || u == 0x11 || u == 0x1D ||
        u == 0x1E || u == 0x1F || u == 0x01)
      continue;
    if (u == 0x1B) { // ESC / CSI
      int j = i + 1;
      if (j < in.size() && in.at(j) == QLatin1Char('[')) {
        ++j;
        while (j < in.size()) {
          const ushort cu = in.at(j).unicode();
          ++j;
          if (cu >= 0x40 && cu <= 0x7E)
            break;
        }
        i = j - 1;
        continue;
      }
      if (j < in.size())
        i = j;
      continue;
    }
    if (u == '\t' || u == '\n' || u == '\r' || u == 0x2028 || u == 0x2029) {
      out.append(QLatin1Char(' '));
      continue;
    }
    if (u < 0x20 || u == 0x7F)
      continue;
    out.append(c);
  }
  return out;
}

static void durableSync(QFile &file) {
  file.flush();
#if defined(Q_OS_UNIX)
  const int fd = file.handle();
  if (fd >= 0)
    ::fsync(fd);
#elif defined(Q_OS_WIN)
  const int fd = file.handle();
  if (fd >= 0) {
    const intptr_t osf = _get_osfhandle(fd);
    if (osf != -1)
      FlushFileBuffers(reinterpret_cast<HANDLE>(osf));
  }
#else
  Q_UNUSED(file);
#endif
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

void Logger::rotateIfNeeded(const QString &filePath) const {
  if (m_maxBackups < 1)
    return;
  if (!QFileInfo::exists(filePath))
    return;
  if (QFileInfo(filePath).size() <= m_maxBytes)
    return;

  const QString oldest = filePath + "." + QString::number(m_maxBackups);
  QFile::remove(oldest);
  for (int i = m_maxBackups - 1; i >= 1; --i) {
    const QString src = filePath + "." + QString::number(i);
    const QString dst = filePath + "." + QString::number(i + 1);
    if (!QFileInfo::exists(src))
      continue;
    QFile::remove(dst);
    QFile::rename(src, dst);
  }
  const QString first = filePath + QStringLiteral(".1");
  QFile::remove(first);
  if (!QFile::rename(filePath, first))
    qWarning("Logger: could not rotate %s", qPrintable(filePath));
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

  const QString filePath = logFilePath(network, channel);
  rotateIfNeeded(filePath);

  QFile file(filePath);
#ifdef Q_OS_UNIX
  const mode_t oldMask = umask(0077);
#endif
  // Binary mode so the newline stays '\n' on Windows. Text mode would write
  // "\r\n", and loadScrollback would keep the '\r' on the message.
  const bool opened = file.open(QIODevice::Append);
#ifdef Q_OS_UNIX
  umask(oldMask);
#endif
  if (!opened) {
    qWarning("Logger: cannot open %s", qPrintable(filePath));
    return;
  }
  restrictToOwner(filePath, false);

  const QString safeType = sanitizeLogText(type);
  const QString safeMessage = sanitizeLogText(message);
  QTextStream stream(&file);
  if (writeTimestamps()) {
    const QString ts =
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    stream << ts << " [" << safeType << "] " << safeMessage << "\n";
  } else {
    stream << "[" << safeType << "] " << safeMessage << "\n";
  }
  stream.flush();
  if (stream.status() != QTextStream::Ok || file.error() != QFile::NoError) {
    qWarning("Logger: write failed for %s", qPrintable(filePath));
    return;
  }
  durableSync(file);
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
    QString line = allLines[i];
    if (line.endsWith(QLatin1Char('\r')))
      line.chop(1);
    if (line.isEmpty())
      continue;
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
