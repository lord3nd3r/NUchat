#pragma once

#include <QString>
#include <QStringList>

class IrcConnection;

// Run Python then Lua hooks. EAT_PLUGIN from an earlier engine skips the
// later one. The returned mask is the OR of every hook that actually ran.
int dispatchScriptServerLine(IrcConnection *conn, const QString &line);
int dispatchScriptCommand(const QString &command, const QStringList &args);
