#include "MessageModel.h"
#include "ImageDownloader.h"
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

MessageModel::MessageModel(QObject *parent) : QAbstractListModel(parent) {
  connect(ImageDownloader::instance(), &ImageDownloader::imageReady, this,
          &MessageModel::onImageReady);
}

int MessageModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_messages.count();
}

QVariant MessageModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.count())
    return {};
  const Message &msg = m_messages.at(index.row());
  switch (role) {
  case TypeRole:
    return msg.type;
  case TextRole:
    return msg.text;
  case TimestampRole:
    return msg.timestamp.toString(Qt::ISODate);
  default:
    return {};
  }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[TypeRole] = "type";
  roles[TextRole] = "text";
  roles[TimestampRole] = "timestamp";
  return roles;
}

void MessageModel::addMessage(const QString &type, const QString &text) {
  beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());
  Message msg;
  msg.type = type;
  msg.text = text;
  msg.timestamp = QDateTime::currentDateTime();
  m_messages.append(msg);
  endInsertRows();
  emit messageAdded(formatLine(msg));

  // Auto-download images for chat/action messages if enabled in preferences
  bool showInlineImages =
      QSettings().value(QStringLiteral("ui/showInlineImages"), true).toBool();
  if (showInlineImages &&
      (type == QLatin1String("chat") || type == QLatin1String("action"))) {
    static const QRegularExpression urlRe(QStringLiteral(
        "(https?://[^\\s\\x02\\x03\\x04\\x0F\\x16\\x1D\\x1E\\x1F]+)"));
    auto it = urlRe.globalMatch(text);
    while (it.hasNext()) {
      QString url = it.next().captured(1);
      // Strip trailing punctuation
      while (url.endsWith('.') || url.endsWith(',') || url.endsWith(';') ||
             url.endsWith(':') || url.endsWith(')') || url.endsWith('\'') ||
             url.endsWith('"'))
        url.chop(1);
      if (ImageDownloader::isImageUrl(url)) {
        m_pendingImages.insert(url);
        ImageDownloader::instance()->download(url);
      }
    }
  }
}

void MessageModel::clear() {
  beginResetModel();
  m_messages.clear();
  endResetModel();
  emit cleared();
}

// Standard mIRC color palette (indices 0-15)
static const char *mircColors[] = {
    "#ffffff", "#000000",
    "#00007f", "#009300", // 0-3: white, black, navy, green
    "#ff0000", "#7f0000",
    "#9c009c", "#fc7f00", // 4-7: red, brown, purple, orange
    "#ffff00", "#00fc00",
    "#009393", "#00ffff", // 8-11: yellow, lime, teal, cyan
    "#0000fc", "#ff00ff",
    "#7f7f7f", "#d2d2d2" // 12-15: blue, pink, grey, light grey
};

// Extended mIRC colors 16-98 (approximated)
static QString mircColor(int idx) {
  if (idx >= 0 && idx <= 15)
    return QString::fromLatin1(mircColors[idx]);
  // Extended colors 16-98: generate from 6x6x6 cube + greys
  if (idx >= 16 && idx <= 87) {
    int n = idx - 16;
    int r = (n / 12) * 51;
    int g = ((n / 2) % 6) * 51;
    int b = (n % 2) * 128 + ((n / 2) % 2) * 127;
    // Better approximation: use standard 6x6x6 cube
    int ri = n / 36, gi = (n / 6) % 6, bi = n % 6;
    r = ri * 51;
    g = gi * 51;
    b = bi * 51;
    return QStringLiteral("#%1%2%3")
        .arg(r, 2, 16, QLatin1Char('0'))
        .arg(g, 2, 16, QLatin1Char('0'))
        .arg(b, 2, 16, QLatin1Char('0'));
  }
  if (idx >= 88 && idx <= 98) {
    int grey = (idx - 88) * 23 + 10;
    return QStringLiteral("#%1%1%1").arg(grey, 2, 16, QLatin1Char('0'));
  }
  return QString();
}

// Strip literal HTML tags from incoming IRC text, but preserve IRC-style
// <nick> patterns.  A "real" HTML tag either:
//   - is a closing tag  (starts with /)
//   - has attributes    (contains whitespace after the tag name)
//   - matches a known HTML element name
// IRC nicks never contain whitespace or slashes, so they are safe.
static const QSet<QString> s_htmlTagNames = {
    "a",          "b",        "i",     "u",      "s",      "em",      "strong",
    "span",       "div",      "p",     "br",     "hr",     "img",     "font",
    "small",      "big",      "sub",   "sup",    "ul",     "ol",      "li",
    "table",      "tr",       "td",    "th",     "thead",  "tbody",   "h1",
    "h2",         "h3",       "h4",    "h5",     "h6",     "pre",     "code",
    "blockquote", "script",   "style", "html",   "head",   "body",    "meta",
    "link",       "title",    "input", "button", "form",   "label",   "select",
    "option",     "textarea", "nav",   "header", "footer", "section", "article",
    "aside",      "main"};

static QString stripHtmlTags(const QString &text) {
  if (!text.contains(QLatin1Char('<')))
    return text;

  QString result;
  result.reserve(text.size());
  int i = 0;
  const int len = text.size();
  while (i < len) {
    if (text[i] != QLatin1Char('<')) {
      result += text[i++];
      continue;
    }
    // Find matching >
    int end = text.indexOf(QLatin1Char('>'), i + 1);
    if (end < 0) {
      // No closing >, keep rest as-is
      result += text.mid(i);
      break;
    }
    const QString inner = text.mid(i + 1, end - i - 1).trimmed();
    // Empty tag — keep
    if (inner.isEmpty()) {
      result += text[i++];
      continue;
    }
    // Closing tag </foo>
    if (inner[0] == QLatin1Char('/')) {
      i = end + 1; // strip it
      continue;
    }
    // Self-closing tag <foo/> or <foo .../>
    if (inner.endsWith(QLatin1Char('/'))) {
      i = end + 1;
      continue;
    }
    // Tag with attributes: contains whitespace after the first word
    int spaceIdx = -1;
    for (int k = 0; k < inner.size(); ++k) {
      if (inner[k].isSpace()) {
        spaceIdx = k;
        break;
      }
    }
    if (spaceIdx >= 0) {
      i = end + 1; // has attributes → strip
      continue;
    }
    // Check against known HTML element names (case-insensitive)
    if (s_htmlTagNames.contains(inner.toLower())) {
      i = end + 1;
      continue;
    }
    // Doesn't look like HTML (e.g. IRC nick <blondie>) — keep the < and advance
    result += text[i++];
  }
  return result;
}

QString MessageModel::ircToHtml(const QString &text) {
  // Strip any literal HTML tags first (relay bots sometimes embed HTML)
  const QString stripped = stripHtmlTags(text);

  // HTML-escape first
  QString escaped;
  escaped.reserve(stripped.size() * 1.2);
  for (const QChar &ch : stripped) {
    if (ch == '<')
      escaped += "&lt;";
    else if (ch == '>')
      escaped += "&gt;";
    else if (ch == '&')
      escaped += "&amp;";
    else if (ch == '"')
      escaped += "&quot;";
    else
      escaped += ch;
  }

  // Parse IRC formatting codes and convert to HTML
  QString result;
  result.reserve(escaped.size() * 1.5);

  bool bold = false, italic = false, underline = false, strikethrough = false;
  QString fgColor, bgColor;
  int openSpans = 0;

  auto closeAllSpans = [&]() {
    for (int s = 0; s < openSpans; s++)
      result += "</span>";
    openSpans = 0;
  };

  auto openColorSpan = [&]() {
    if (fgColor.isEmpty() && bgColor.isEmpty())
      return;
    result += "<span style=\"";
    if (!fgColor.isEmpty())
      result += "color:" + fgColor + ";";
    if (!bgColor.isEmpty())
      result += "background-color:" + bgColor + ";";
    result += "\">";
    openSpans++;
  };

  int i = 0;
  int len = escaped.length();
  while (i < len) {
    QChar ch = escaped[i];
    ushort code = ch.unicode();

    if (code == 0x02) { // Bold
      if (bold)
        result += "</b>";
      else
        result += "<b>";
      bold = !bold;
      i++;
    } else if (code == 0x1D) { // Italic
      if (italic)
        result += "</i>";
      else
        result += "<i>";
      italic = !italic;
      i++;
    } else if (code == 0x1F) { // Underline
      if (underline)
        result += "</u>";
      else
        result += "<u>";
      underline = !underline;
      i++;
    } else if (code == 0x1E) { // Strikethrough
      if (strikethrough)
        result += "</s>";
      else
        result += "<s>";
      strikethrough = !strikethrough;
      i++;
    } else if (code == 0x16) { // Reverse — swap fg/bg
      closeAllSpans();
      std::swap(fgColor, bgColor);
      if (fgColor.isEmpty() && !bgColor.isEmpty())
        fgColor = "#000000";
      openColorSpan();
      i++;
    } else if (code == 0x0F) { // Reset
      closeAllSpans();
      if (bold) {
        result += "</b>";
        bold = false;
      }
      if (italic) {
        result += "</i>";
        italic = false;
      }
      if (underline) {
        result += "</u>";
        underline = false;
      }
      if (strikethrough) {
        result += "</s>";
        strikethrough = false;
      }
      fgColor.clear();
      bgColor.clear();
      i++;
    } else if (code == 0x03) { // Color: \x03FG[,BG]
      i++;                     // skip \x03
      closeAllSpans();

      // Parse foreground color number (1-2 digits)
      int fg = -1, bg = -1;
      if (i < len && escaped[i].isDigit()) {
        fg = escaped[i].digitValue();
        i++;
        if (i < len && escaped[i].isDigit()) {
          fg = fg * 10 + escaped[i].digitValue();
          i++;
        }
        // Parse optional background
        if (i < len && escaped[i] == ',') {
          i++; // skip comma
          if (i < len && escaped[i].isDigit()) {
            bg = escaped[i].digitValue();
            i++;
            if (i < len && escaped[i].isDigit()) {
              bg = bg * 10 + escaped[i].digitValue();
              i++;
            }
          }
        }
      }

      if (fg >= 0) {
        fgColor = mircColor(fg);
        if (bg >= 0)
          bgColor = mircColor(bg);
      } else {
        // Bare \x03 resets color
        fgColor.clear();
        bgColor.clear();
      }
      openColorSpan();
    } else if (code == 0x04) { // Hex color: \x04RRGGBB[,RRGGBB]
      i++;                     // skip \x04
      closeAllSpans();
      // Parse 6-char hex fg
      if (i + 5 < len) {
        bool ok;
        QString hexFg = escaped.mid(i, 6);
        hexFg.toInt(&ok, 16);
        if (ok) {
          fgColor = "#" + hexFg;
          i += 6;
          if (i < len && escaped[i] == ',') {
            i++;
            if (i + 5 < len) {
              QString hexBg = escaped.mid(i, 6);
              hexBg.toInt(&ok, 16);
              if (ok) {
                bgColor = "#" + hexBg;
                i += 6;
              }
            }
          }
        }
      }
      openColorSpan();
    } else if (code == 0x11) { // Monospace
      // Not widely used; skip the control char
      i++;
    } else {
      result += ch;
      i++;
    }
  }

  // Close remaining tags
  closeAllSpans();
  if (bold)
    result += "</b>";
  if (italic)
    result += "</i>";
  if (underline)
    result += "</u>";
  if (strikethrough)
    result += "</s>";

  return result;
}

// ── URL linkification ──
// Runs as a post-pass on the HTML output from ircToHtml().
// Detects http/https URLs in text content and wraps them in <a> tags.
QString MessageModel::linkifyUrls(const QString &html) {
  // Match URLs that are NOT already inside an HTML tag (our tags only use
  // style attributes with color/background values, never http URLs).
  static const QRegularExpression re(QStringLiteral("(https?://[^\\s<\"]+)"));

  QString result;
  result.reserve(html.size() + 256);
  int lastPos = 0;

  auto it = re.globalMatch(html);
  while (it.hasNext()) {
    auto m = it.next();
    QString url = m.captured(1);

    // Strip trailing HTML entities that were part of surrounding text
    while (url.endsWith(QLatin1String("&amp;")) ||
           url.endsWith(QLatin1String("&lt;")) ||
           url.endsWith(QLatin1String("&gt;"))) {
      if (url.endsWith(QLatin1String("&amp;")))
        url.chop(5);
      else
        url.chop(4);
    }
    // Strip trailing punctuation
    while (url.endsWith('.') || url.endsWith(',') || url.endsWith(';') ||
           url.endsWith(':') || url.endsWith(')') || url.endsWith('\''))
      url.chop(1);

    result += html.mid(lastPos, m.capturedStart(1) - lastPos);

    // Restore HTML entities for the href attribute
    QString href = url;
    href.replace(QLatin1String("&amp;"), QLatin1String("&"));
    href.replace(QLatin1String("&lt;"), QLatin1String("<"));
    href.replace(QLatin1String("&gt;"), QLatin1String(">"));
    href.replace(QLatin1String("&quot;"), QLatin1String("\""));

    // Build the link
    result += QStringLiteral("<a href=\"") + href.toHtmlEscaped() +
              QStringLiteral(
                  "\" style=\"color:#4fc3f7; text-decoration:underline;\">") +
              url + QStringLiteral("</a>");

    // For video URLs, add a label
    if (ImageDownloader::isVideoUrl(href)) {
      result += QStringLiteral(
          " <span style=\"color:#4fc3f7;\">&#9654; Video</span>");
    }

    lastPos = m.capturedStart(1) + url.length();
  }
  result += html.mid(lastPos);
  return result;
}

// ── Image download callback ──
void MessageModel::onImageReady(const QString &url, const QString &localPath,
                                int width, int height) {
  if (!m_pendingImages.contains(url))
    return;
  m_pendingImages.remove(url);

  int displayW = qMin(width, 400);
  int displayH = (width > 0) ? (height * displayW / width) : 0;

  // Build an embed message with clickable image
  QString html = QStringLiteral("<a href=\"") + url.toHtmlEscaped() +
                 QStringLiteral("\"><img src=\"file://") + localPath +
                 QStringLiteral("\" width=\"%1\" height=\"%2\"></a>")
                     .arg(displayW)
                     .arg(displayH);

  beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());
  Message msg;
  msg.type = QStringLiteral("embed");
  msg.text = html;
  msg.timestamp = QDateTime::currentDateTime();
  m_messages.append(msg);
  endInsertRows();
  emit messageAdded(html);
}

// ── Nick colorization ──
// 16 distinct nick colors – dark palette (bright on dark bg)
static const char *nickPaletteDark[] = {
    "#c678dd", // purple
    "#e06c75", // red
    "#98c379", // green
    "#e5c07b", // yellow
    "#61afef", // blue
    "#56b6c2", // cyan
    "#be5046", // dark red
    "#d19a66", // orange
    "#ff79c6", // pink
    "#50fa7b", // bright green
    "#8be9fd", // bright cyan
    "#bd93f9", // lavender
    "#ffb86c", // light orange
    "#ff5555", // bright red
    "#69ff94", // mint
    "#f1fa8c", // light yellow
};
// 16 distinct nick colors – light palette (dark/saturated on light bg)
static const char *nickPaletteLight[] = {
    "#7b2fa0", // purple
    "#b5182e", // red
    "#3a7d1a", // green
    "#9e7c16", // dark yellow
    "#1a5fb4", // blue
    "#1a8a7d", // teal
    "#8b2d1a", // dark red
    "#a85600", // orange
    "#c4246e", // pink
    "#1d8348", // forest green
    "#0e6f87", // dark cyan
    "#6a3daa", // dark lavender
    "#b46a00", // dark orange
    "#cc2222", // bright red
    "#127a3a", // dark mint
    "#887a09", // olive
};
static const int nickColorCount = 16;
static bool s_darkMode = true;

void MessageModel::setDarkMode(bool dark) { s_darkMode = dark; }

QString MessageModel::nickColor(const QString &nick) {
  // DJB2 hash for consistent color assignment
  uint hash = 5381;
  for (const QChar &c : nick)
    hash = ((hash << 5) + hash) + c.toLower().unicode();
  const char **palette = s_darkMode ? nickPaletteDark : nickPaletteLight;
  return QString::fromLatin1(palette[hash % nickColorCount]);
}

// Finds <nick> and <@nick> patterns in HTML-escaped text and wraps them
// in colored, clickable <a> tags with nick:// scheme.
// Also handles action lines: "* nick ..." at the start.
QString MessageModel::colorizeNicks(const QString &html) {
  // Match: &lt;[prefix]nick&gt; — the < > are HTML-escaped by ircToHtml
  static const QRegularExpression nickRe(
      QStringLiteral("&lt;([~&amp;@%+]*)([^&]+?)&gt;"));

  QString result;
  result.reserve(html.size() + 256);
  int lastPos = 0;
  bool firstMatch = true;

  auto it = nickRe.globalMatch(html);
  while (it.hasNext()) {
    auto m = it.next();
    result += html.mid(lastPos, m.capturedStart() - lastPos);

    QString prefix = m.captured(1);
    QString bareNick = m.captured(2);
    // Unescape &amp; back to & in prefix (e.g. &amp; for & prefix)
    prefix.replace(QLatin1String("&amp;"), QLatin1String("&"));
    QString color = nickColor(bareNick);

    result += QStringLiteral("&lt;") + prefix +
              QStringLiteral("<a href=\"nick://") + bareNick.toHtmlEscaped() +
              QStringLiteral("\" style=\"color:") + color +
              QStringLiteral("; text-decoration:none; font-weight:bold;\">") +
              bareNick.toHtmlEscaped() + QStringLiteral("</a>") +
              QStringLiteral("&gt;");

    lastPos = m.capturedEnd();
    firstMatch = false;
  }
  result += html.mid(lastPos);

  // Also colorize action nicks: "* nick " at line start
  // Pattern: after "* " at start, the next word is the nick
  static const QRegularExpression actionRe(QStringLiteral("^(\\* )([^\\s<]+)"));
  auto am = actionRe.match(result);
  if (am.hasMatch()) {
    QString actionNick = am.captured(2);
    QString color = nickColor(actionNick);
    QString replacement =
        am.captured(1) + QStringLiteral("<a href=\"nick://") +
        actionNick.toHtmlEscaped() + QStringLiteral("\" style=\"color:") +
        color + QStringLiteral("; text-decoration:none; font-weight:bold;\">") +
        actionNick.toHtmlEscaped() + QStringLiteral("</a>");
    result = replacement + result.mid(am.capturedEnd());
  }

  return result;
}

QString MessageModel::formatLine(const Message &msg) {
  // Embed messages are pre-formatted HTML — pass through
  if (msg.type == QLatin1String("embed"))
    return msg.text;

  QString ts = QStringLiteral("<span style=\"color:#888;\">[") +
               msg.timestamp.toString(QStringLiteral("hh:mm:ss")) +
               QStringLiteral("]</span> ");
  QString prefix;
  if (msg.type == QLatin1String("system"))
    prefix = QStringLiteral("<span style=\"color:#888;\">*** </span>");
  else if (msg.type == QLatin1String("action"))
    prefix = QStringLiteral("<span style=\"color:#ce9178;\">* </span>");
  else if (msg.type == QLatin1String("error"))
    prefix = QStringLiteral("<span style=\"color:#f44747;\">! </span>");

  return ts + prefix + linkifyUrls(colorizeNicks(ircToHtml(msg.text)));
}

QString MessageModel::allFormattedText() const {
  QStringList lines;
  lines.reserve(m_messages.size());
  for (const auto &msg : m_messages)
    lines.append(formatLine(msg));
  return lines.join(QStringLiteral("<br>"));
}
