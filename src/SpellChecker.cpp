#include "SpellChecker.h"
#include "SpellDict.h"
#include <QDebug>
#include <QFile>
#include <hunspell.hxx>
#include <string>
#include <vector>

SpellChecker::SpellChecker(const QString &lang, QObject *parent)
    : QObject(parent), m_hunspell(nullptr) {
  const QStringList roots = spellDictionaryRoots();
  const QPair<QString, QString> found = locateHunspellDictionary(lang, roots);
  if (!found.first.isEmpty() && !found.second.isEmpty()) {
    m_hunspell = new Hunspell(found.second.toLocal8Bit().constData(),
                              found.first.toLocal8Bit().constData());
  } else {
    qWarning() << "SpellChecker: Could not find dictionary for" << lang
               << "in" << roots;
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
