#pragma once

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>

class Hunspell;

class SpellChecker : public QObject {
  Q_OBJECT
public:
  explicit SpellChecker(const QString &lang = QStringLiteral("en_US"),
                        QObject *parent = nullptr);
  ~SpellChecker() override;

  Q_INVOKABLE bool isCorrect(const QString &word);
  Q_INVOKABLE QStringList suggestions(const QString &word);

private:
  Hunspell *m_hunspell;
  QMutex m_mutex;
};
