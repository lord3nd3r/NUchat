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
  case FormattedTextRole:
    return msg.formattedText;
  default:
    return {};
  }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[TypeRole] = "type";
  roles[TextRole] = "text";
  roles[TimestampRole] = "timestamp";
  roles[FormattedTextRole] = "formattedText";
  return roles;
}

void MessageModel::addMessage(const QString &type, const QString &text,
                              const QString &timestamp) {
  beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());
  Message msg;
  msg.type = type;
  msg.text = text;

  if (!timestamp.isEmpty()) {
    msg.timestamp = QDateTime::fromString(timestamp, Qt::ISODate);
    if (!msg.timestamp.isValid()) {
      msg.timestamp = QDateTime::currentDateTime();
    }
  } else {
    msg.timestamp = QDateTime::currentDateTime();
  }

  msg.formattedText = formatLine(msg); // pre-render HTML once
  m_messages.append(msg);
  endInsertRows();
  if (!m_batchMode)
    emit messageAdded(msg.formattedText); // reuse cached text, no double format

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
  m_expandedGroups.clear();
  // Drop pending image downloads — otherwise an image requested in the
  // previous channel would embed into whichever channel is shown when the
  // download finishes.
  m_pendingImages.clear();
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
  // Extended colors 16-98: generate from standard 6x6x6 cube + greys
  if (idx >= 16 && idx <= 87) {
    int n = idx - 16;
    int r = (n / 36) * 51;
    int g = ((n / 6) % 6) * 51;
    int b = (n % 6) * 51;
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

// Strip literal HTML tags from incoming IRC text, but only real HTML tags.
// IRC messages use <nick> patterns which must not be removed.
// A "real" HTML tag has attributes (contains a space/slash) or matches a
// known HTML element name.
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

static bool isHtmlTag(const QString &tagContent) {
  if (tagContent.isEmpty())
    return false;
  // Closing tag: </tagname>
  QString inner = tagContent;
  if (inner.startsWith('/'))
    inner = inner.mid(1);
  // Get the tag name (up to first space or end)
  int spacePos = inner.indexOf(' ');
  QString tagName = (spacePos >= 0) ? inner.left(spacePos) : inner;
  tagName = tagName.toLower();
  // If it has attributes (space present) it's definitely HTML
  if (spacePos >= 0)
    return true;
  // If it matches a known HTML element name, it's HTML
  return s_htmlTagNames.contains(tagName);
}

static QString stripHtmlTags(const QString &text) {
  if (!text.contains(QLatin1Char('<')))
    return text;

  QString result;
  result.reserve(text.size());
  int i = 0;
  int len = text.size();
  while (i < len) {
    if (text[i] == '<') {
      // Find the closing >
      int end = text.indexOf('>', i + 1);
      if (end < 0) {
        // No closing >, keep everything as-is
        result += text.mid(i);
        break;
      }
      QString tagContent = text.mid(i + 1, end - i - 1).trimmed();
      if (isHtmlTag(tagContent)) {
        // Skip this tag entirely
        i = end + 1;
      } else {
        // Not an HTML tag (e.g. IRC nick like <blondie>) — keep it
        result += text[i];
        i++;
      }
    } else {
      result += text[i];
      i++;
    }
  }
  return result;
}

// Wrap emoji grapheme clusters with an explicit emoji-capable fallback stack.
// This keeps the main chat font intact (including monospace) while ensuring
// emoji symbols render with available color/emoji fonts.
static bool isEmojiBaseCodepoint(char32_t cp) {
  if (cp >= 0x1F300 && cp <= 0x1FAFF)
    return true;
  if (cp >= 0x1F1E6 && cp <= 0x1F1FF)
    return true; // regional indicators (flags)
  if (cp >= 0x2600 && cp <= 0x27BF)
    return true;
  if (cp >= 0x2300 && cp <= 0x23FF)
    return true;
  if (cp == 0x00A9 || cp == 0x00AE || cp == 0x203C || cp == 0x2049 ||
      cp == 0x2122 || cp == 0x2139 || cp == 0x3030 || cp == 0x303D ||
      cp == 0x3297 || cp == 0x3299)
    return true;
  return false;
}

static bool isEmojiJoinerOrModifier(char32_t cp) {
  return cp == 0x200D || cp == 0xFE0E || cp == 0xFE0F || cp == 0x20E3 ||
         (cp >= 0x1F3FB && cp <= 0x1F3FF);
}

static QString wrapEmojiWithFontFallback(const QString &html) {
  static const QString kEmojiSpanPrefix = QStringLiteral(
      "<span style=\"font-family:'Noto Color Emoji','Segoe UI Emoji','Apple "
      "Color Emoji','Noto Emoji';\">");
  static const QString kEmojiSpanSuffix = QStringLiteral("</span>");

  QString out;
  out.reserve(html.size() + 64);

  auto appendCodepoint = [&out](char32_t cp) {
    if (cp <= 0xFFFF) {
      out += QChar(static_cast<ushort>(cp));
    } else {
      cp -= 0x10000;
      const ushort hi = static_cast<ushort>(0xD800 + ((cp >> 10) & 0x3FF));
      const ushort lo = static_cast<ushort>(0xDC00 + (cp & 0x3FF));
      out += QChar(hi);
      out += QChar(lo);
    }
  };

  int i = 0;
  while (i < html.size()) {
    const QChar ch = html.at(i);

    // Preserve HTML tags/entities verbatim.
    if (ch == QLatin1Char('<')) {
      const int end = html.indexOf(QLatin1Char('>'), i + 1);
      if (end < 0) {
        out += html.mid(i);
        break;
      }
      out += html.mid(i, end - i + 1);
      i = end + 1;
      continue;
    }
    if (ch == QLatin1Char('&')) {
      const int end = html.indexOf(QLatin1Char(';'), i + 1);
      if (end > i) {
        out += html.mid(i, end - i + 1);
        i = end + 1;
        continue;
      }
    }

    char32_t cp = ch.unicode();
    int cpLen = 1;
    if (ch.isHighSurrogate() && i + 1 < html.size() &&
        html.at(i + 1).isLowSurrogate()) {
      cp = QChar::surrogateToUcs4(ch, html.at(i + 1));
      cpLen = 2;
    }

    if (!isEmojiBaseCodepoint(cp)) {
      out += html.mid(i, cpLen);
      i += cpLen;
      continue;
    }

    out += kEmojiSpanPrefix;
    appendCodepoint(cp);
    i += cpLen;

    // Include ZWJ/VS/modifier-linked continuation codepoints in one span.
    while (i < html.size()) {
      const QChar n = html.at(i);
      if (n == QLatin1Char('<') || n == QLatin1Char('&'))
        break;

      char32_t ncp = n.unicode();
      int nLen = 1;
      if (n.isHighSurrogate() && i + 1 < html.size() &&
          html.at(i + 1).isLowSurrogate()) {
        ncp = QChar::surrogateToUcs4(n, html.at(i + 1));
        nLen = 2;
      }

      if (!isEmojiJoinerOrModifier(ncp) && !isEmojiBaseCodepoint(ncp))
        break;

      appendCodepoint(ncp);
      i += nLen;
    }

    out += kEmojiSpanSuffix;
  }

  return out;
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
  int openFonts = 0;

  auto closeAllFonts = [&]() {
    for (int s = 0; s < openFonts; s++)
      result += "</font>";
    openFonts = 0;
  };

  auto openColorFont = [&]() {
    if (fgColor.isEmpty() && bgColor.isEmpty())
      return;
    if (!fgColor.isEmpty()) {
      result += "<font color=\"" + fgColor + "\">";
      openFonts++;
    }
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
      closeAllFonts();
      std::swap(fgColor, bgColor);
      if (fgColor.isEmpty() && !bgColor.isEmpty())
        fgColor = "#000000";
      openColorFont();
      i++;
    } else if (code == 0x0F) { // Reset
      closeAllFonts();
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
      closeAllFonts();

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
      openColorFont();
    } else if (code == 0x04) { // Hex color: \x04RRGGBB[,RRGGBB]
      i++;                     // skip \x04
      closeAllFonts();
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
      openColorFont();
    } else if (code == 0x11) { // Monospace
      // Not widely used; skip the control char
      i++;
    } else {
      result += ch;
      i++;
    }
  }

  // Close remaining tags
  closeAllFonts();
  if (bold)
    result += "</b>";
  if (italic)
    result += "</i>";
  if (underline)
    result += "</u>";
  if (strikethrough)
    result += "</s>";

  return wrapEmojiWithFontFallback(result);
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
      result += QStringLiteral(" <font color=\"#4fc3f7\">&#9654; Video</font>");
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

void MessageModel::setNickname(const QString &nick) { m_nickname = nick; }

bool MessageModel::containsNickWord(const QString &text, const QString &nick) {
  if (nick.isEmpty())
    return false;
  // Characters that can appear in an IRC nick (RFC 2812 + common extensions);
  // a mention is the nick NOT surrounded by these on either side.
  static const QString nickChars =
      QStringLiteral("A-Za-z0-9_\\-\\[\\]\\\\`^{}|");
  const QRegularExpression re(
      QStringLiteral("(?<![%1])%2(?![%1])")
          .arg(nickChars, QRegularExpression::escape(nick)),
      QRegularExpression::CaseInsensitiveOption);
  return re.match(text).hasMatch();
}

void MessageModel::setHighlightEnabled(bool enabled) {
  m_highlightEnabled = enabled;
}

void MessageModel::setTimestampFormat(const QString &fmt) {
  m_timestampFormat = fmt.isEmpty() ? QStringLiteral("hh:mm:ss") : fmt;
}

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
  // The nick part must NOT contain < or > (which would indicate we're
  // spanning across HTML tags from mIRC color wrapping)
  // Channel mode prefixes: ~ (owner), & (admin/protected), @ (op), % (halfop), + (voice)
  // Use alternation to match &amp; as a complete entity, not individual chars
  static const QRegularExpression nickRe(
      QStringLiteral("&lt;((?:~|&amp;|@|%|\\+)*)([^&<>]+?)&gt;"));

  QString result;
  result.reserve(html.size() + 256);
  int lastPos = 0;

  auto it = nickRe.globalMatch(html);
  while (it.hasNext()) {
    auto m = it.next();

    // Extra safety: if the matched region contains any HTML tags, skip it
    QString matched = m.captured(0);
    if (matched.contains(QLatin1Char('<')) ||
        matched.contains(QLatin1Char('>'))) {
      result += html.mid(lastPos, m.capturedEnd() - lastPos);
      lastPos = m.capturedEnd();
      continue;
    }

    result += html.mid(lastPos, m.capturedStart() - lastPos);

    QString prefix = m.captured(1);
    QString bareNick = m.captured(2);
    // Unescape &amp; back to & in prefix (e.g. &amp; for & prefix)
    prefix.replace(QLatin1String("&amp;"), QLatin1String("&"));
    QString color = nickColor(bareNick);

    // Re-escape the prefix for HTML output
    QString escapedPrefix = prefix;
    escapedPrefix.replace(QLatin1String("&"), QLatin1String("&amp;"));
    
    result += QStringLiteral("&lt;") + escapedPrefix +
              QStringLiteral("<a href=\"nick://") + bareNick.toHtmlEscaped() +
              QStringLiteral("\" style=\"color:") + color +
              QStringLiteral("; text-decoration:none; font-weight:bold;\">") +
              bareNick.toHtmlEscaped() + QStringLiteral("</a>") +
              QStringLiteral("&gt;");

    lastPos = m.capturedEnd();
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

QString MessageModel::formatLineFromQml(const QString &text,
                                        const QString &type,
                                        const QString &timestamp) const {
  Message msg;
  msg.text = text;
  msg.type = type;
  msg.timestamp = QDateTime::fromString(timestamp, Qt::ISODate);
  if (!msg.timestamp.isValid())
    msg.timestamp = QDateTime::currentDateTime();
  return formatLine(msg);
}

QString MessageModel::formatLine(const Message &msg) const {
  // Embed messages are pre-formatted HTML — pass through
  if (msg.type == QLatin1String("embed"))
    return msg.text;

  QString ts = QStringLiteral("<font color=\"#888888\">[") +
               msg.timestamp.toString(m_timestampFormat) +
               QStringLiteral("]</font> ");
  QString prefix;
  if (msg.type == QLatin1String("system"))
    prefix = QStringLiteral("<font color=\"#888888\">*** </font>");
  else if (msg.type == QLatin1String("action"))
    prefix = QStringLiteral("<font color=\"#ce9178\">* </font>");
  else if (msg.type == QLatin1String("error"))
    prefix = QStringLiteral("<font color=\"#f44747\">! </font>");

  QString htmlBody = linkifyUrls(colorizeNicks(ircToHtml(msg.text)));

  // Highlight other people's chat messages that mention our nick
  bool isHighlight = false;
  if (m_highlightEnabled && !m_nickname.isEmpty() &&
      msg.type == QLatin1String("chat")) {
    // Extract sender nick from "<[+@~&%]nick> ..." format
    int lt = msg.text.indexOf(QLatin1Char('<'));
    int gt = msg.text.indexOf(QLatin1Char('>'));
    if (lt >= 0 && gt > lt) {
      QString senderRaw = msg.text.mid(lt + 1, gt - lt - 1);
      // Strip mode prefixes (+, @, ~, &, %)
      while (!senderRaw.isEmpty() &&
             (senderRaw[0] == '+' || senderRaw[0] == '@' ||
              senderRaw[0] == '~' || senderRaw[0] == '&' ||
              senderRaw[0] == '%'))
        senderRaw = senderRaw.mid(1);

      // Only highlight if the sender is NOT us
      if (senderRaw.compare(m_nickname, Qt::CaseInsensitive) != 0) {
        // Check the body after the <nick> prefix for our nick (whole word)
        QString msgBody = msg.text.mid(gt + 1);
        isHighlight = containsNickWord(msgBody, m_nickname);
      }
    }
  }

  if (isHighlight) {
    return QStringLiteral("<span style=\"background-color:#3a2a00;\">") + ts +
           prefix + htmlBody +
           QStringLiteral("</span>") +
           QStringLiteral("<span style=\"background-color:transparent;\">&#8203;</span>");
  }

  return ts + prefix + htmlBody;
}

// ─── Event-collapse helpers ──────────────────────────────────────────────────
namespace {
enum class EventKind { Join, Part, Quit, Kick, Nick, Mode, Unknown };

static EventKind classifyEvent(const QString &text, QString &outNick) {
  int sp = text.indexOf(QLatin1Char(' '));
  outNick = sp > 0 ? text.left(sp) : text;
  if (text.contains(QLatin1String(" has joined ")))      return EventKind::Join;
  if (text.contains(QLatin1String(" has left ")))        return EventKind::Part;
  if (text.contains(QLatin1String(" has quit")))         return EventKind::Quit;
  if (text.contains(QLatin1String(" was kicked by ")))   return EventKind::Kick;
  if (text.contains(QLatin1String(" is now known as "))) return EventKind::Nick;
  if (text.contains(QLatin1String(" sets mode ")))       return EventKind::Mode;
  return EventKind::Unknown;
}

static bool isCollapsibleEvent(const MessageModel::Message &msg) {
  if (msg.type != QLatin1String("system")) return false;
  QString nick;
  return classifyEvent(msg.text, nick) != EventKind::Unknown;
}

static QString makeEventGroupHtml(const QList<MessageModel::Message> &msgs,
                                   int from, int count, int groupId, bool expanded) {
  QStringList joins, parts, quits, kicks, nicks;
  int modes = 0;
  for (int i = from; i < from + count; ++i) {
    QString nick;
    switch (classifyEvent(msgs.at(i).text, nick)) {
    case EventKind::Join:  joins.append(nick); break;
    case EventKind::Part:  parts.append(nick); break;
    case EventKind::Quit:  quits.append(nick); break;
    case EventKind::Kick:  kicks.append(nick); break;
    case EventKind::Nick:  nicks.append(nick); break;
    case EventKind::Mode:  ++modes;             break;
    default:                                    break;
    }
  }
  QStringList summary;
  if (!joins.isEmpty()) summary << joins.join(QStringLiteral(", ")) + QStringLiteral(" joined");
  if (!parts.isEmpty()) summary << parts.join(QStringLiteral(", ")) + QStringLiteral(" left");
  if (!quits.isEmpty()) summary << quits.join(QStringLiteral(", ")) + QStringLiteral(" quit");
  if (!kicks.isEmpty()) summary << kicks.join(QStringLiteral(", ")) + QStringLiteral(" kicked");
  if (!nicks.isEmpty()) summary << nicks.join(QStringLiteral(", ")) + QStringLiteral(" changed nick");
  if (modes > 0)        summary << QString::number(modes) + QStringLiteral(" mode change(s)");

  // Timestamp: single minute or a range
  const QDateTime t0  = msgs.at(from).timestamp;
  const QDateTime t1  = msgs.at(from + count - 1).timestamp;
  const QString   fmt = QStringLiteral("hh:mm");
  const QString tsStr = (t0.toString(fmt) == t1.toString(fmt))
      ? QStringLiteral("[") + t0.toString(fmt) + QStringLiteral("]")
      : QStringLiteral("[") + t0.toString(fmt) + QStringLiteral(" \u2013 ") + t1.toString(fmt) + QStringLiteral("]");

  QString summaryText = summary.join(QStringLiteral(" \u00b7 ")) +
                         QStringLiteral(" (") + QString::number(count) + QStringLiteral(" events)");

  QString arrow = expanded ? QStringLiteral("\u25bc") : QStringLiteral("\u25b6");

  // Wrap in a clickable link so the user can expand the group
  return QStringLiteral("<font color=\"#888888\">") + tsStr +
         QStringLiteral("</font> <font color=\"#888888\">*** </font>"
                        "<a href=\"eventgroup://") +
         QString::number(groupId) +
         QStringLiteral("\" style=\"color:#6a9955; text-decoration:none;\">") +
         arrow + QStringLiteral(" ") +
         summaryText +
         QStringLiteral("</a>");
}
} // anonymous namespace
// ─────────────────────────────────────────────────────────────────────────────

void MessageModel::toggleEventGroup(int groupId) {
  if (m_expandedGroups.contains(groupId)) {
    m_expandedGroups.remove(groupId);
  } else {
    m_expandedGroups.insert(groupId);
  }
}

QString MessageModel::allFormattedText() const {
  static const int kCollapseThreshold = 3;
  const bool collapseEvents =
      QSettings().value(QStringLiteral("ui/collapseEvents"), true).toBool();

  QStringList result;
  result.reserve(m_messages.size());

  int i = 0;
  while (i < m_messages.size()) {
    if (collapseEvents && isCollapsibleEvent(m_messages.at(i))) {
      int j = i + 1;
      while (j < m_messages.size() && isCollapsibleEvent(m_messages.at(j)))
        ++j;
      const int count = j - i;
      if (count >= kCollapseThreshold) {
        int gid = i; // Use start index as stable group ID
        bool expanded = m_expandedGroups.contains(gid);
        
        result.append(makeEventGroupHtml(m_messages, i, count, gid, expanded));
        
        if (expanded) {
          for (int k = i; k < j; ++k) {
            // Indent the individual lines
            result.append(QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;") + m_messages.at(k).formattedText);
          }
        }
      } else {
        for (int k = i; k < j; ++k)
          result.append(m_messages.at(k).formattedText);
      }
      i = j;
    } else {
      result.append(m_messages.at(i).formattedText);
      ++i;
    }
  }
  return result.join(QStringLiteral("<br>"));
}

void MessageModel::beginBatch() {
  m_batchMode = true;
}

void MessageModel::endBatch() {
  m_batchMode = false;
  emit reloaded();
}
