#include "CredentialRedactor.h"

#include <initializer_list>

namespace CredentialRedactor {

namespace {

bool equalsOneOf(const QString &value, std::initializer_list<const char *> names) {
    for (const char *name : names) {
        if (value.compare(QLatin1String(name), Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

bool isCredentialCommand(const QString &cmd) {
    return equalsOneOf(cmd, {
        "IDENTIFY", "ID", "SIDENTIFY", "AUTH", "LOGIN",
        "GHOST", "RELEASE", "RECOVER", "REGAIN",
        "REGISTER", "REG", "GROUP", "DROP", "UNGROUP"
    });
}

bool isSaslMechanism(const QString &arg) {
    return equalsOneOf(arg, {
        "PLAIN", "EXTERNAL", "ANONYMOUS", "LOGIN",
        "SCRAM-SHA-1", "SCRAM-SHA-256", "SCRAM-SHA-512",
        "ECDSA-NIST256P-CHALLENGE", "ECDSA-NIST256P",
        "DH-BLOWFISH", "DH-AES"
    });
}

struct Parts {
    QString tags;    // includes the trailing space when present
    QString prefix;  // includes the trailing space when present
    QString command;
    QString args;
    bool hasArgs = false;
};

Parts splitLine(const QString &line) {
    Parts p;
    QString work = line;
    if (work.startsWith(QLatin1Char('@'))) {
        int sp = work.indexOf(QLatin1Char(' '));
        if (sp < 0) {
            p.command = work;
            return p;
        }
        p.tags = work.left(sp + 1);
        work = work.mid(sp + 1);
    }
    if (work.startsWith(QLatin1Char(':'))) {
        int sp = work.indexOf(QLatin1Char(' '));
        if (sp < 0) {
            p.command = work;
            return p;
        }
        p.prefix = work.left(sp + 1);
        work = work.mid(sp + 1);
    }
    int sp = work.indexOf(QLatin1Char(' '));
    if (sp < 0) {
        p.command = work;
        return p;
    }
    p.command = work.left(sp);
    p.args = work.mid(sp + 1);
    p.hasArgs = true;
    return p;
}

QString joinLine(const Parts &p, const QString &args, bool withArgs) {
    QString out = p.tags + p.prefix + p.command;
    if (withArgs)
        out += QLatin1Char(' ') + args;
    return out;
}

QString redactOperArgs(const QString &args) {
    int colon = args.indexOf(QStringLiteral(" :"));
    if (colon >= 0)
        return args.left(colon) + QStringLiteral(" :***");
    if (args.startsWith(QLatin1Char(':')))
        return QStringLiteral(":***");
    int sp = args.lastIndexOf(QLatin1Char(' '));
    if (sp < 0)
        return QStringLiteral("***");
    return args.left(sp) + QStringLiteral(" ***");
}

QString redactPrivmsgArgs(const QString &args, bool *changed) {
    *changed = false;
    if (args.isEmpty())
        return args;
    QString target = args.section(QLatin1Char(' '), 0, 0);
    if (!isServiceNick(target))
        return args;

    int colon = args.indexOf(QStringLiteral(" :"));
    QString head;
    QString msg;
    if (colon >= 0) {
        head = args.left(colon + 2); // "NickServ :"
        msg = args.mid(colon + 2);
    } else {
        int sp = args.indexOf(QLatin1Char(' '));
        if (sp < 0)
            return args;
        head = args.left(sp + 1);
        msg = args.mid(sp + 1);
    }
    QString redacted = redactServiceMessage(msg);
    if (redacted == msg)
        return args;
    *changed = true;
    return head + redacted;
}

} // namespace

bool isServiceNick(const QString &target) {
    QString nick = target.trimmed();
    int at = nick.indexOf(QLatin1Char('@'));
    if (at >= 0)
        nick = nick.left(at);
    int bang = nick.indexOf(QLatin1Char('!'));
    if (bang >= 0)
        nick = nick.left(bang);
    return equalsOneOf(nick, {
        "NickServ", "ChanServ", "OperServ", "HostServ", "MemoServ",
        "BotServ", "HelpServ", "SaslServ", "AuthServ", "UserServ",
        "GameServ", "StatServ", "InfoServ",
        "Q", "X" // QuakeNet Q, Undernet X
    });
}

QString redactServiceMessage(const QString &message) {
    int start = 0;
    while (start < message.size() && message.at(start).isSpace())
        ++start;
    if (start >= message.size())
        return message;

    int sp = message.indexOf(QLatin1Char(' '), start);
    QString cmd = (sp < 0) ? message.mid(start) : message.mid(start, sp - start);
    QString cmdUp = cmd.toUpper();

    auto withRestRedacted = [&]() {
        return message.left(start) + cmd + QStringLiteral(" ***");
    };

    if (isCredentialCommand(cmdUp)) {
        if (sp < 0)
            return message; // "IDENTIFY" with no argument carries no secret
        return withRestRedacted();
    }

    if (cmdUp == QLatin1String("SET") && sp >= 0) {
        QString rest = message.mid(sp + 1);
        int restStart = 0;
        while (restStart < rest.size() && rest.at(restStart).isSpace())
            ++restStart;
        int subEnd = rest.indexOf(QLatin1Char(' '), restStart);
        QString sub = (subEnd < 0) ? rest.mid(restStart)
                                   : rest.mid(restStart, subEnd - restStart);
        QString subUp = sub.toUpper();
        if (subUp == QLatin1String("PASSWORD") || subUp == QLatin1String("PASSWD")) {
            if (subEnd < 0)
                return message;
            return message.left(start) + cmd + QLatin1Char(' ') + sub
                   + QStringLiteral(" ***");
        }
    }
    return message;
}

QString redactOutgoingChat(const QString &target, const QString &message) {
    if (!isServiceNick(target))
        return message;
    return redactServiceMessage(message);
}

QString redactIrcLine(const QString &line) {
    if (line.isEmpty())
        return line;
    Parts p = splitLine(line);
    QString cmd = p.command.toUpper();

    if (cmd == QLatin1String("PASS")) {
        if (!p.hasArgs)
            return line;
        return joinLine(p, QStringLiteral("***"), true);
    }

    if (cmd == QLatin1String("AUTHENTICATE")) {
        if (!p.hasArgs)
            return line;
        QString arg = p.args.trimmed();
        if (arg.isEmpty() || arg == QLatin1String("+") || arg == QLatin1String("*"))
            return line;
        if (isSaslMechanism(arg))
            return line;
        return joinLine(p, QStringLiteral("***"), true);
    }

    if (cmd == QLatin1String("OPER")) {
        if (!p.hasArgs)
            return line;
        return joinLine(p, redactOperArgs(p.args), true);
    }

    if (cmd == QLatin1String("PRIVMSG") || cmd == QLatin1String("NOTICE")) {
        if (!p.hasArgs)
            return line;
        bool changed = false;
        QString args = redactPrivmsgArgs(p.args, &changed);
        if (!changed)
            return line;
        return joinLine(p, args, true);
    }

    return line;
}

} // namespace CredentialRedactor
