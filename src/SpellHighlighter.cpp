#include "SpellHighlighter.h"
#include "SpellChecker.h"
#include <QTextDocument>

SpellHighlighter::SpellHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent), m_quickDoc(nullptr), m_checker(nullptr),
      m_enabled(true) {
  m_errorFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
  m_errorFormat.setUnderlineColor(Qt::red);
}

SpellHighlighter::~SpellHighlighter() {}

QQuickTextDocument *SpellHighlighter::textDocument() const {
  return m_quickDoc;
}

void SpellHighlighter::setTextDocument(QQuickTextDocument *doc) {
  if (m_quickDoc == doc)
    return;

  m_quickDoc = doc;
  setDocument(doc ? doc->textDocument() : nullptr);
  emit textDocumentChanged();
}

bool SpellHighlighter::enabled() const { return m_enabled; }

void SpellHighlighter::setEnabled(bool enabled) {
  if (m_enabled == enabled)
    return;

  m_enabled = enabled;
  emit enabledChanged();
  rehighlight();
}

SpellChecker *SpellHighlighter::spellChecker() const { return m_checker; }

void SpellHighlighter::setSpellChecker(SpellChecker *checker) {
  if (m_checker == checker)
    return;

  m_checker = checker;
  emit spellCheckerChanged();
  rehighlight();
}

void SpellHighlighter::highlightBlock(const QString &text) {
  if (!m_enabled || !m_checker)
    return;

  int length = text.length();
  int wordStart = -1;

  for (int i = 0; i <= length; ++i) {
    bool isAlpha = i < length && text.at(i).isLetter();
    if (isAlpha) {
      if (wordStart == -1) {
        wordStart = i;
      }
    } else {
      if (wordStart != -1) {
        QString word = text.mid(wordStart, i - wordStart);
        if (!m_checker->isCorrect(word)) {
          setFormat(wordStart, i - wordStart, m_errorFormat);
        }
        wordStart = -1;
      }
    }
  }
}
