#pragma once

#include <QString>
#include <QStringList>

// Lightweight view of one IRC line for script and plugin dispatch.
// IRCv3 tags (a leading "@..." token) are removed before the command is
// chosen. The optional ":prefix" is skipped as well, so both
// "PING :server" and "@time=... :nick PRIVMSG #c :hi" yield a real command.
struct IrcLineView {
    QString tags;       // tag blob without the leading '@'; empty if none
    QString prefix;     // prefix without the leading ':'; empty if none
    QString command;    // upper-case command or numeric; empty if absent
    QStringList parts;  // space-split of the line after tags are removed
};

inline IrcLineView parseIrcLine(const QString &raw) {
    IrcLineView view;
    QString rest = raw;
    if (rest.startsWith(QLatin1Char('@'))) {
        const int sp = rest.indexOf(QLatin1Char(' '));
        if (sp < 0) {
            view.tags = rest.mid(1);
            return view;
        }
        view.tags = rest.mid(1, sp - 1);
        rest = rest.mid(sp + 1);
    }

    view.parts = rest.split(QLatin1Char(' '));
    int idx = 0;
    if (!view.parts.isEmpty() && view.parts[0].startsWith(QLatin1Char(':'))) {
        view.prefix = view.parts[0].mid(1);
        idx = 1;
    }
    while (idx < view.parts.size() && view.parts[idx].isEmpty())
        ++idx;
    if (idx < view.parts.size())
        view.command = view.parts[idx].toUpper();
    return view;
}

// Nick portion of "nick!user@host" (or the whole prefix when there is no bang).
inline QString ircNickFromPrefix(const QString &prefix) {
    const int bang = prefix.indexOf(QLatin1Char('!'));
    return bang >= 0 ? prefix.left(bang) : prefix;
}

// Trailing parameter (the text after the first " :"), with tags already
// stripped. Empty when the line has no trailing parameter.
inline QString ircTrailing(const QString &raw) {
    QString rest = raw;
    if (rest.startsWith(QLatin1Char('@'))) {
        const int sp = rest.indexOf(QLatin1Char(' '));
        if (sp < 0)
            return {};
        rest = rest.mid(sp + 1);
    }
    const int idx = rest.indexOf(QLatin1String(" :"));
    if (idx < 0)
        return {};
    return rest.mid(idx + 2);
}

// "!command arg..." at the start of a PRIVMSG/NOTICE body.
// Command is lower-case. Empty command means the line is not a bang command.
struct BangCommand {
    QString command;
    QStringList args;
};

inline BangCommand bangCommandFromRaw(const QString &raw) {
    BangCommand bang;
    const IrcLineView view = parseIrcLine(raw);
    if (view.command != QLatin1String("PRIVMSG") &&
        view.command != QLatin1String("NOTICE"))
        return bang;

    const QString text = ircTrailing(raw);
    // CTCP (and ACTION) bodies start with 0x01, never a bang command.
    if (text.size() < 2 || text.at(0) == QChar(0x01) || text.at(0) != QLatin1Char('!'))
        return bang;

    const QStringList words = text.mid(1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.isEmpty())
        return bang;
    bang.command = words.first().toLower();
    bang.args = words.mid(1);
    return bang;
}
