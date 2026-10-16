#include "SpellDict.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace {

void addUnique(QStringList &roots, const QString &path) {
  if (path.isEmpty())
    return;
  const QString clean = QDir::cleanPath(path);
  if (!roots.contains(clean))
    roots.append(clean);
}

} // namespace

QStringList spellDictionaryRoots() {
  QStringList roots;

  const QString dicPath = qEnvironmentVariable("DICPATH");
  if (!dicPath.isEmpty()) {
#ifdef Q_OS_WIN
    const QChar sep = QLatin1Char(';');
#else
    const QChar sep = QLatin1Char(':');
#endif
    const QStringList extra = dicPath.split(sep, Qt::SkipEmptyParts);
    for (const QString &dir : extra)
      addUnique(roots, dir.trimmed());
  }

  const QString appDir = QCoreApplication::applicationDirPath();
  addUnique(roots, appDir + QStringLiteral("/hunspell"));
  addUnique(roots, appDir + QStringLiteral("/dictionaries"));

#ifdef Q_OS_MAC
  addUnique(roots, appDir + QStringLiteral("/../Resources/hunspell"));
  addUnique(roots, appDir + QStringLiteral("/../Resources/dictionaries"));
  addUnique(roots, QStringLiteral("/opt/homebrew/share/hunspell"));
  addUnique(roots, QStringLiteral("/usr/local/share/hunspell"));
  addUnique(roots, QStringLiteral("/Library/Spelling"));
  addUnique(roots, QDir::homePath() + QStringLiteral("/Library/Spelling"));
#endif

#ifdef Q_OS_WIN
  const QString local =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  addUnique(roots, local + QStringLiteral("/hunspell"));
  const QString programFiles = qEnvironmentVariable("ProgramFiles");
  addUnique(roots, programFiles + QStringLiteral("/LibreOffice/share/wordbook"));
#endif

#if defined(Q_OS_UNIX)
  addUnique(roots, QStringLiteral("/usr/share/hunspell"));
  addUnique(roots, QStringLiteral("/usr/share/myspell"));
  addUnique(roots, QStringLiteral("/usr/share/myspell/dicts"));
  addUnique(roots, QStringLiteral("/usr/local/share/hunspell"));
  addUnique(roots, QStringLiteral("/usr/local/share/myspell/dicts"));
#endif

  const QString generic =
      QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  addUnique(roots, generic + QStringLiteral("/hunspell"));
  addUnique(roots, generic + QStringLiteral("/myspell/dicts"));

  const QString appData =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  addUnique(roots, appData + QStringLiteral("/hunspell"));

  return roots;
}

QPair<QString, QString> locateHunspellDictionary(const QString &lang,
                                                 const QStringList &roots) {
  if (lang.isEmpty())
    return {};
  QStringList names;
  names << lang;
  if (lang.contains(QLatin1Char('_')))
    names << QString(lang).replace(QLatin1Char('_'), QLatin1Char('-'));
  else if (lang.contains(QLatin1Char('-')))
    names << QString(lang).replace(QLatin1Char('-'), QLatin1Char('_'));

  for (const QString &root : roots) {
    for (const QString &name : names) {
      const QString dic = QDir(root).filePath(name + QStringLiteral(".dic"));
      const QString aff = QDir(root).filePath(name + QStringLiteral(".aff"));
      if (QFile::exists(dic) && QFile::exists(aff))
        return {dic, aff};
    }
  }
  return {};
}
