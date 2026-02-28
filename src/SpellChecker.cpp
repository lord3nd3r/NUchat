#include "SpellChecker.h"
#include <QDebug>
#include <QFile>
#include <hunspell/hunspell.hxx>
#include <string>
#include <vector>

SpellChecker::SpellChecker(const QString &lang, QObject *parent)
    : QObject(parent), m_hunspell(nullptr) {
  // Common paths in Linux for Hunspell dictionaries
  QString dicPath =
      QStringLiteral("/usr/share/hunspell/") + lang + QStringLiteral(".dic");
  QString affPath =
      QStringLiteral("/usr/share/hunspell/") + lang + QStringLiteral(".aff");

  if (QFile::exists(dicPath) && QFile::exists(affPath)) {
    m_hunspell = new Hunspell(affPath.toLocal8Bit().constData(),
                              dicPath.toLocal8Bit().constData());
  } else {
    qWarning() << "SpellChecker: Could not find dictionary for" << lang << "at"
               << dicPath;
  }
}

SpellChecker::~SpellChecker() { delete m_hunspell; }

bool SpellChecker::isCorrect(const QString &word) {
  QMutexLocker locker(&m_mutex);
  if (!m_hunspell || word.isEmpty())
    return true;

  std::string utf8Word = word.toStdString();
  bool result = m_hunspell->spell(utf8Word);
  return result;
}

QStringList SpellChecker::suggestions(const QString &word) {
  QMutexLocker locker(&m_mutex);
  if (!m_hunspell || word.isEmpty())
    return QStringList();

  std::string utf8Word = word.toStdString();
  std::vector<std::string> sugList = m_hunspell->suggest(utf8Word);

  QStringList result;
  for (const std::string &sug : sugList) {
    result.append(QString::fromStdString(sug));
  }

  return result;
}
