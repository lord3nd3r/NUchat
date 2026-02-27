#include "MessageModel.h"
#include "ImageDownloader.h"
#include <QRegularExpression>
#include <QUrl>

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(ImageDownloader::instance(), &ImageDownloader::imageReady,
            this, &MessageModel::onImageReady);
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_messages.count();
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
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

QHash<int, QByteArray> MessageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TypeRole] = "type";
    roles[TextRole] = "text";
    roles[TimestampRole] = "timestamp";
    return roles;
}

void MessageModel::addMessage(const QString &type, const QString &text)
{
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());
    Message msg;
    msg.type = type;
    msg.text = text;
    msg.timestamp = QDateTime::currentDateTime();
    m_messages.append(msg);
    endInsertRows();
    emit messageAdded(formatLine(msg));

    // Auto-download images for chat/action messages
    if (type == QLatin1String("chat") || type == QLatin1String("action")) {
        static const QRegularExpression urlRe(
            QStringLiteral("(https?://[^\\s\\x02\\x03\\x04\\x0F\\x16\\x1D\\x1E\\x1F]+)"));
        auto it = urlRe.globalMatch(text);
        while (it.hasNext()) {
            QString url = it.next().captured(1);
            // Strip trailing punctuation
            while (url.endsWith('.') || url.endsWith(',') || url.endsWith(';')
                   || url.endsWith(':') || url.endsWith(')') || url.endsWith('\'')
                   || url.endsWith('"'))
                url.chop(1);
            if (ImageDownloader::isImageUrl(url)) {
                m_pendingImages.insert(url);
                ImageDownloader::instance()->download(url);
            }
        }
    }
}

void MessageModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
    emit cleared();
}

// Standard mIRC color palette (indices 0-15)
static const char *mircColors[] = {
    "#ffffff", "#000000", "#00007f", "#009300",  // 0-3: white, black, navy, green
    "#ff0000", "#7f0000", "#9c009c", "#fc7f00",  // 4-7: red, brown, purple, orange
    "#ffff00", "#00fc00", "#009393", "#00ffff",  // 8-11: yellow, lime, teal, cyan
    "#0000fc", "#ff00ff", "#7f7f7f", "#d2d2d2"   // 12-15: blue, pink, grey, light grey
};

// Extended mIRC colors 16-98 (approximated)
static QString mircColor(int idx)
{
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
        r = ri * 51; g = gi * 51; b = bi * 51;
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

QString MessageModel::ircToHtml(const QString &text)
{
    // HTML-escape first
    QString escaped;
    escaped.reserve(text.size() * 1.2);
    for (const QChar &ch : text) {
        if (ch == '<') escaped += "&lt;";
        else if (ch == '>') escaped += "&gt;";
        else if (ch == '&') escaped += "&amp;";
        else if (ch == '"') escaped += "&quot;";
        else escaped += ch;
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
        if (fgColor.isEmpty() && bgColor.isEmpty()) return;
        result += "<span style=\"";
        if (!fgColor.isEmpty()) result += "color:" + fgColor + ";";
        if (!bgColor.isEmpty()) result += "background-color:" + bgColor + ";";
        result += "\">";
        openSpans++;
    };

    int i = 0;
    int len = escaped.length();
    while (i < len) {
        QChar ch = escaped[i];
        ushort code = ch.unicode();

        if (code == 0x02) { // Bold
            if (bold) result += "</b>";
            else result += "<b>";
            bold = !bold;
            i++;
        } else if (code == 0x1D) { // Italic
            if (italic) result += "</i>";
            else result += "<i>";
            italic = !italic;
            i++;
        } else if (code == 0x1F) { // Underline
            if (underline) result += "</u>";
            else result += "<u>";
            underline = !underline;
            i++;
        } else if (code == 0x1E) { // Strikethrough
            if (strikethrough) result += "</s>";
            else result += "<s>";
            strikethrough = !strikethrough;
            i++;
        } else if (code == 0x16) { // Reverse — swap fg/bg
            closeAllSpans();
            std::swap(fgColor, bgColor);
            if (fgColor.isEmpty() && !bgColor.isEmpty()) fgColor = "#000000";
            openColorSpan();
            i++;
        } else if (code == 0x0F) { // Reset
            closeAllSpans();
            if (bold) { result += "</b>"; bold = false; }
            if (italic) { result += "</i>"; italic = false; }
            if (underline) { result += "</u>"; underline = false; }
            if (strikethrough) { result += "</s>"; strikethrough = false; }
            fgColor.clear();
            bgColor.clear();
            i++;
        } else if (code == 0x03) { // Color: \x03FG[,BG]
            i++; // skip \x03
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
                if (bg >= 0) bgColor = mircColor(bg);
            } else {
                // Bare \x03 resets color
                fgColor.clear();
                bgColor.clear();
            }
            openColorSpan();
        } else if (code == 0x04) { // Hex color: \x04RRGGBB[,RRGGBB]
            i++; // skip \x04
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
    if (bold) result += "</b>";
    if (italic) result += "</i>";
    if (underline) result += "</u>";
    if (strikethrough) result += "</s>";

    return result;
}

// ── URL linkification ──
// Runs as a post-pass on the HTML output from ircToHtml().
// Detects http/https URLs in text content and wraps them in <a> tags.
QString MessageModel::linkifyUrls(const QString &html)
{
    // Match URLs that are NOT already inside an HTML tag (our tags only use
    // style attributes with color/background values, never http URLs).
    static const QRegularExpression re(
        QStringLiteral("(https?://[^\\s<\"]+)"));

    QString result;
    result.reserve(html.size() + 256);
    int lastPos = 0;

    auto it = re.globalMatch(html);
    while (it.hasNext()) {
        auto m = it.next();
        QString url = m.captured(1);

        // Strip trailing HTML entities that were part of surrounding text
        while (url.endsWith(QLatin1String("&amp;"))
               || url.endsWith(QLatin1String("&lt;"))
               || url.endsWith(QLatin1String("&gt;")))
        {
            if (url.endsWith(QLatin1String("&amp;")))  url.chop(5);
            else                                        url.chop(4);
        }
        // Strip trailing punctuation
        while (url.endsWith('.') || url.endsWith(',') || url.endsWith(';')
               || url.endsWith(':') || url.endsWith(')') || url.endsWith('\''))
            url.chop(1);

        result += html.mid(lastPos, m.capturedStart(1) - lastPos);

        // Restore HTML entities for the href attribute
        QString href = url;
        href.replace(QLatin1String("&amp;"), QLatin1String("&"));
        href.replace(QLatin1String("&lt;"),  QLatin1String("<"));
        href.replace(QLatin1String("&gt;"),  QLatin1String(">"));
        href.replace(QLatin1String("&quot;"), QLatin1String("\""));

        // Build the link
        result += QStringLiteral("<a href=\"")
                  + href.toHtmlEscaped()
                  + QStringLiteral("\" style=\"color:#4fc3f7; text-decoration:underline;\">")
                  + url
                  + QStringLiteral("</a>");

        // For image URLs, add an inline preview if cached
        if (ImageDownloader::isImageUrl(href)) {
            if (ImageDownloader::instance()->isCached(href)) {
                QString localPath = ImageDownloader::instance()->cachedPath(href);
                result += QStringLiteral("<br><a href=\"") + href.toHtmlEscaped()
                          + QStringLiteral("\"><img src=\"file://")
                          + localPath
                          + QStringLiteral("\" width=\"400\"></a>");
            }
        }
        // For video URLs, add a label
        if (ImageDownloader::isVideoUrl(href)) {
            result += QStringLiteral(" <span style=\"color:#4fc3f7;\">&#9654; Video</span>");
        }

        lastPos = m.capturedStart(1) + url.length();
    }
    result += html.mid(lastPos);
    return result;
}

// ── Image download callback ──
void MessageModel::onImageReady(const QString &url, const QString &localPath,
                                int width, int height)
{
    if (!m_pendingImages.contains(url))
        return;
    m_pendingImages.remove(url);

    int displayW = qMin(width, 400);
    int displayH = (width > 0) ? (height * displayW / width) : 0;

    // Build an embed message with clickable image
    QString html = QStringLiteral("<a href=\"")
                   + url.toHtmlEscaped()
                   + QStringLiteral("\"><img src=\"file://")
                   + localPath
                   + QStringLiteral("\" width=\"%1\" height=\"%2\"></a>")
                     .arg(displayW).arg(displayH);

    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());
    Message msg;
    msg.type = QStringLiteral("embed");
    msg.text = html;
    msg.timestamp = QDateTime::currentDateTime();
    m_messages.append(msg);
    endInsertRows();
    emit messageAdded(html);
}

QString MessageModel::formatLine(const Message &msg)
{
    // Embed messages are pre-formatted HTML — pass through
    if (msg.type == QLatin1String("embed"))
        return msg.text;

    QString ts = QStringLiteral("<span style=\"color:#888;\">[")
                 + msg.timestamp.toString(QStringLiteral("hh:mm:ss"))
                 + QStringLiteral("]</span> ");
    QString prefix;
    if (msg.type == QLatin1String("system"))
        prefix = QStringLiteral("<span style=\"color:#888;\">*** </span>");
    else if (msg.type == QLatin1String("action"))
        prefix = QStringLiteral("<span style=\"color:#ce9178;\">* </span>");
    else if (msg.type == QLatin1String("error"))
        prefix = QStringLiteral("<span style=\"color:#f44747;\">! </span>");

    return ts + prefix + linkifyUrls(ircToHtml(msg.text));
}

QString MessageModel::allFormattedText() const
{
    QStringList lines;
    lines.reserve(m_messages.size());
    for (const auto &msg : m_messages)
        lines.append(formatLine(msg));
    return lines.join(QStringLiteral("<br>"));
}
