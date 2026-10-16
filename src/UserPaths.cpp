#include "UserPaths.h"
#include "Settings.h"

#include <QDir>
#include <QStandardPaths>

#ifdef Q_OS_UNIX
#include <pwd.h>
#include <unistd.h>
#endif

QString expandHomePath(const QString &path) {
  if (path.isEmpty() || path[0] != QLatin1Char('~'))
    return path;

  if (path.size() == 1 || path[1] == QLatin1Char('/') ||
      path[1] == QLatin1Char('\\')) {
    QString rest = path.mid(1);
    if (rest.startsWith(QLatin1Char('/')) || rest.startsWith(QLatin1Char('\\')))
      rest = rest.mid(1);
    return QDir::cleanPath(QDir::homePath() + QLatin1Char('/') + rest);
  }

#ifdef Q_OS_UNIX
  const int slash = path.indexOf(QLatin1Char('/'));
  const QString user = slash < 0 ? path.mid(1) : path.mid(1, slash - 1);
  const QString rest = slash < 0 ? QString() : path.mid(slash + 1);
  const QByteArray uname = user.toLocal8Bit();
  struct passwd *pw = ::getpwnam(uname.constData());
  if (!pw || !pw->pw_dir)
    return path;
  return QDir::cleanPath(QString::fromLocal8Bit(pw->pw_dir) + QLatin1Char('/') +
                          rest);
#else
  return path;
#endif
}

qint64 dccMaxFileBytes(const Settings *settings) {
  constexpr qint64 kDefault = 100LL * 1024 * 1024;
  if (!settings)
    return kDefault;
  if (settings->contains(QStringLiteral("dcc/maxSize"))) {
    const qint64 mb = settings->getInt(QStringLiteral("dcc/maxSize"), 0);
    if (mb <= 0)
      return 0; // 0 = unlimited, matching the preference label
    return mb * 1024 * 1024;
  }
  if (settings->contains(QStringLiteral("dcc/maxFileSize")))
    return settings->value(QStringLiteral("dcc/maxFileSize"), kDefault)
        .toLongLong();
  return kDefault;
}

QString dccDownloadDirectory(const Settings *settings) {
  const QString fallback = QDir::homePath() + QStringLiteral("/Downloads");
  if (!settings)
    return fallback;
  const QString dir = settings->getString(QStringLiteral("dcc/downloadDir"));
  if (dir.isEmpty())
    return fallback;
  return expandHomePath(dir);
}
