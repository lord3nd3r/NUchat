#include "MessageParser.h"
#include <QHash>

// Map of lowercase alias → canonical command (also lowercase).
// Only aliases that differ from the canonical name are listed.
static const QHash<QString, QString> &aliasMap() {
    static const QHash<QString, QString> map = {
        // General
        {"j",          "join"},
        {"leave",      "part"},
        {"p",          "part"},
        {"privmsg",    "msg"},
        {"m",          "msg"},
        {"query",      "msg"},
        {"q",          "msg"},
        {"disconnect", "quit"},
        {"bye",        "quit"},
        {"rejoin",     "cycle"},
        {"quote",      "raw"},
        // Information
        {"w",          "whois"},
        {"wi",         "whois"},
        {"t",          "topic"},
        // User modes
        {"hop",        "halfop"},
        {"dehop",      "dehalfop"},
        {"v",          "voice"},
        // Services
        {"nickserv",   "ns"},
        {"chanserv",   "cs"},
        {"operserv",   "os"},
        {"hostserv",   "hs"},
        {"memoserv",   "ms"},
        {"botserv",    "bs"},
        // Channel management
        {"b",          "ban"},
        {"inv",        "invite"},
        {"kickban",    "kb"},
    };
    return map;
}

QString MessageParser::parse(const QString &raw) {
    if (!raw.startsWith('/'))
        return raw;

    // Split into command and rest (preserving leading '/')
    const int spaceIdx = raw.indexOf(' ');
    const QString cmdPart = (spaceIdx >= 0) ? raw.left(spaceIdx) : raw;
    const QString rest    = (spaceIdx >= 0) ? raw.mid(spaceIdx)  : QString();

    const QString cmdLower = cmdPart.mid(1).toLower(); // strip leading '/'
    const auto it = aliasMap().constFind(cmdLower);
    if (it != aliasMap().constEnd())
        return '/' + it.value().toUpper() + rest;

    return raw;
}
