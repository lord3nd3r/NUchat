#include "ScriptDispatch.h"
#include "ScriptEat.h"

#include <QtGlobal>

class IrcConnection;

#ifdef HAVE_PYTHON
#include "PythonScriptEngine.h"
#endif
#ifdef HAVE_LUA
#include "LuaScriptEngine.h"
#endif

int dispatchScriptServerLine(IrcConnection *conn, const QString &line) {
    int eat = NUCHAT_EAT_NONE;
#ifdef HAVE_PYTHON
    if (auto *py = PythonScriptEngine::instance())
        eat = foldEat(eat, py->handleServerLine(conn, line));
#endif
#ifdef HAVE_LUA
    if (!eatStopsHooks(eat)) {
        if (auto *lua = LuaScriptEngine::instance())
            eat = foldEat(eat, lua->handleServerLine(conn, line));
    }
#else
    Q_UNUSED(conn);
    Q_UNUSED(line);
#endif
#ifndef HAVE_PYTHON
    Q_UNUSED(conn);
    Q_UNUSED(line);
#endif
    return eat;
}

int dispatchScriptCommand(const QString &command, const QStringList &args) {
    int eat = NUCHAT_EAT_NONE;
#ifdef HAVE_PYTHON
    if (auto *py = PythonScriptEngine::instance())
        eat = foldEat(eat, py->handleCommand(command, args));
#endif
#ifdef HAVE_LUA
    if (!eatStopsHooks(eat)) {
        if (auto *lua = LuaScriptEngine::instance())
            eat = foldEat(eat, lua->handleCommand(command, args));
    }
#else
    Q_UNUSED(command);
    Q_UNUSED(args);
#endif
#ifndef HAVE_PYTHON
    Q_UNUSED(command);
    Q_UNUSED(args);
#endif
    return eat;
}
