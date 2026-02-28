#pragma once

#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class SpellChecker;

class SpellHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
  Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument WRITE
                 setTextDocument NOTIFY textDocumentChanged)
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(SpellChecker *spellChecker READ spellChecker WRITE setSpellChecker
                 NOTIFY spellCheckerChanged)

public:
  explicit SpellHighlighter(QObject *parent = nullptr);
  ~SpellHighlighter() override;

  QQuickTextDocument *textDocument() const;
  void setTextDocument(QQuickTextDocument *doc);

  bool enabled() const;
  void setEnabled(bool enabled);

  SpellChecker *spellChecker() const;
  void setSpellChecker(SpellChecker *checker);

signals:
  void textDocumentChanged();
  void enabledChanged();
  void spellCheckerChanged();

protected:
  void highlightBlock(const QString &text) override;

private:
  QQuickTextDocument *m_quickDoc;
  SpellChecker *m_checker;
  bool m_enabled;
  QTextCharFormat m_errorFormat;
};
