extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "LuaScriptEngine.h"
#include "IRCConnectionManager.h"
#include "IrcConnection.h"
#include "MessageModel.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>

LuaScriptEngine *LuaScriptEngine::s_instance = nullptr;

// Eat constants (same as HexChat/Python engine)
static constexpr int EAT_NONE    = 0;
static constexpr int EAT_HEXCHAT = 1;
static constexpr int EAT_PLUGIN  = 2;
static constexpr int EAT_ALL     = 3;

// Priority constants (HexChat compat)
static constexpr int PRI_HIGHEST = 127;
static constexpr int PRI_HIGH    = 64;
static constexpr int PRI_NORM    = 0;
static constexpr int PRI_LOW     = -64;
static constexpr int PRI_LOWEST  = -128;

// ──────────────────────────────────────────────────────────────
//  Lua C API functions exposed as hexchat.*/nuchat.* to scripts
// ──────────────────────────────────────────────────────────────

static int lua_nuchat_command(lua_State *L)
{
    const char *cmd = luaL_checkstring(L, 1);
    if (LuaScriptEngine::instance())
        LuaScriptEngine::instance()->command(QString::fromUtf8(cmd));
    return 0;
}

static int lua_nuchat_prnt(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    if (LuaScriptEngine::instance())
        LuaScriptEngine::instance()->prnt(QString::fromUtf8(text));
    return 0;
}

static int lua_nuchat_get_info(lua_State *L)
{
    const char *id = luaL_checkstring(L, 1);
    if (LuaScriptEngine::instance()) {
        QString val = LuaScriptEngine::instance()->getInfo(QString::fromUtf8(id));
        if (!val.isNull()) {
            lua_pushstring(L, val.toUtf8().constData());
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

static int lua_nuchat_hook_command(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // Push callback and get ref
    lua_pushvalue(L, 2);
    int cbRef = luaL_ref(L, LUA_REGISTRYINDEX);

    // Optional userdata (arg 3)
    int udRef = LUA_NOREF;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
        lua_pushvalue(L, 3);
        udRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    // Optional priority (arg 4)
    int priority = 0;
    if (lua_gettop(L) >= 4)
        priority = (int)luaL_optinteger(L, 4, 0);

    if (LuaScriptEngine::instance()) {
        int id = LuaScriptEngine::instance()->hookCommand(
            QString::fromUtf8(name).toUpper(), cbRef, udRef, priority);
        lua_pushinteger(L, id);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int lua_nuchat_hook_server(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);
    int cbRef = luaL_ref(L, LUA_REGISTRYINDEX);

    int udRef = LUA_NOREF;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
        lua_pushvalue(L, 3);
        udRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    int priority = 0;
    if (lua_gettop(L) >= 4)
        priority = (int)luaL_optinteger(L, 4, 0);

    if (LuaScriptEngine::instance()) {
        int id = LuaScriptEngine::instance()->hookServer(
            QString::fromUtf8(name).toUpper(), cbRef, udRef, priority);
        lua_pushinteger(L, id);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int lua_nuchat_hook_timer(lua_State *L)
{
    int timeout = (int)luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);
    int cbRef = luaL_ref(L, LUA_REGISTRYINDEX);

    int udRef = LUA_NOREF;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
        lua_pushvalue(L, 3);
        udRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    if (LuaScriptEngine::instance()) {
        int id = LuaScriptEngine::instance()->hookTimer(timeout, cbRef, udRef);
        lua_pushinteger(L, id);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int lua_nuchat_hook_print(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_pushvalue(L, 2);
    int cbRef = luaL_ref(L, LUA_REGISTRYINDEX);

    int udRef = LUA_NOREF;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
        lua_pushvalue(L, 3);
        udRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    int priority = 0;
    if (lua_gettop(L) >= 4)
        priority = (int)luaL_optinteger(L, 4, 0);

    if (LuaScriptEngine::instance()) {
        int id = LuaScriptEngine::instance()->hookPrint(
            QString::fromUtf8(name), cbRef, udRef, priority);
        lua_pushinteger(L, id);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int lua_nuchat_emit_print(lua_State *L)
{
    const char *event = luaL_checkstring(L, 1);
    // Collect remaining string arguments
    int nargs = lua_gettop(L);
    QStringList args;
    for (int i = 2; i <= nargs; ++i) {
        if (lua_isstring(L, i))
            args << QString::fromUtf8(lua_tostring(L, i));
    }
    // TODO: full emit_print dispatch; for now just print the event text
    if (LuaScriptEngine::instance() && !args.isEmpty())
        LuaScriptEngine::instance()->prnt(args.join(" "));
    lua_pushboolean(L, 1);
    return 1;
}

static int lua_nuchat_nickcmp(lua_State *L)
{
    const char *a = luaL_checkstring(L, 1);
    const char *b = luaL_checkstring(L, 2);
    // IRC nick comparison is case-insensitive
    lua_pushinteger(L, QString::fromUtf8(a).compare(QString::fromUtf8(b), Qt::CaseInsensitive));
    return 1;
}

static int lua_nuchat_unhook(lua_State *L)
{
    int hookId = (int)luaL_checkinteger(L, 1);
    if (LuaScriptEngine::instance())
        LuaScriptEngine::instance()->unhook(hookId);
    return 0;
}

// ──────────────────────────────────────────────────────────────
//  LuaScriptEngine implementation
// ──────────────────────────────────────────────────────────────

LuaScriptEngine::LuaScriptEngine(IRCConnectionManager *mgr, QObject *parent)
    : QObject(parent), m_mgr(mgr)
{
    s_instance = this;
    initLua();
}

LuaScriptEngine::~LuaScriptEngine()
{
    shutdownLua();
    if (s_instance == this)
        s_instance = nullptr;
}

LuaScriptEngine *LuaScriptEngine::instance()
{
    return s_instance;
}

void LuaScriptEngine::initLua()
{
    m_L = luaL_newstate();
    luaL_openlibs(m_L);
    registerAPI();
}

void LuaScriptEngine::shutdownLua()
{
    // Clean up timer hooks
    for (auto &h : m_hooks) {
        if (h.timer) {
            h.timer->stop();
            delete h.timer;
        }
    }
    m_hooks.clear();

    if (m_L) {
        lua_close(m_L);
        m_L = nullptr;
    }
}

void LuaScriptEngine::registerAPI()
{
    // Create the API table with HexChat-compatible functions
    lua_newtable(m_L);

    // Register functions (matching HexChat Lua plugin API)
    static const luaL_Reg funcs[] = {
        {"command",      lua_nuchat_command},
        {"prnt",         lua_nuchat_prnt},
        {"print",        lua_nuchat_prnt},        // HexChat compat alias
        {"emit_print",   lua_nuchat_emit_print},
        {"get_info",     lua_nuchat_get_info},
        {"hook_command", lua_nuchat_hook_command},
        {"hook_server",  lua_nuchat_hook_server},
        {"hook_print",   lua_nuchat_hook_print},
        {"hook_timer",   lua_nuchat_hook_timer},
        {"unhook",       lua_nuchat_unhook},
        {"nickcmp",      lua_nuchat_nickcmp},
        {nullptr, nullptr}
    };

    for (const luaL_Reg *f = funcs; f->name; ++f) {
        lua_pushcfunction(m_L, f->func);
        lua_setfield(m_L, -2, f->name);
    }

    // Eat constants — matching HexChat
    lua_pushinteger(m_L, EAT_NONE);    lua_setfield(m_L, -2, "EAT_NONE");
    lua_pushinteger(m_L, EAT_HEXCHAT); lua_setfield(m_L, -2, "EAT_HEXCHAT");
    lua_pushinteger(m_L, EAT_HEXCHAT); lua_setfield(m_L, -2, "EAT_XCHAT");  // legacy compat
    lua_pushinteger(m_L, EAT_PLUGIN);  lua_setfield(m_L, -2, "EAT_PLUGIN");
    lua_pushinteger(m_L, EAT_ALL);     lua_setfield(m_L, -2, "EAT_ALL");
    lua_pushinteger(m_L, PRI_HIGHEST); lua_setfield(m_L, -2, "PRI_HIGHEST");
    lua_pushinteger(m_L, PRI_HIGH);    lua_setfield(m_L, -2, "PRI_HIGH");
    lua_pushinteger(m_L, PRI_NORM);    lua_setfield(m_L, -2, "PRI_NORM");
    lua_pushinteger(m_L, PRI_LOW);     lua_setfield(m_L, -2, "PRI_LOW");
    lua_pushinteger(m_L, PRI_LOWEST);  lua_setfield(m_L, -2, "PRI_LOWEST");

    // Set as both "hexchat" (HexChat compat) and "nuchat" (native)
    lua_pushvalue(m_L, -1);            // duplicate the table
    lua_setglobal(m_L, "hexchat");     // hexchat = table (HexChat backwards compat)
    lua_setglobal(m_L, "nuchat");      // nuchat  = table (NUchat native)
}

void LuaScriptEngine::loadScripts(const QString &directory)
{
    m_directory = directory;
    QDir dir(directory);
    if (!dir.exists())
        dir.mkpath(".");

    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(directory);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &LuaScriptEngine::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        // New files may have appeared
        QDir d(m_directory);
        for (const QString &file : d.entryList({"*.lua"}, QDir::Files)) {
            if (!m_loadedScripts.contains(file)) {
                loadSingleScript(d.filePath(file));
                if (m_watcher) m_watcher->addPath(d.filePath(file));
            }
        }
    });

    for (const QString &file : dir.entryList({"*.lua"}, QDir::Files)) {
        QString path = dir.filePath(file);
        loadSingleScript(path);
        m_watcher->addPath(path);
    }
}

void LuaScriptEngine::loadScript(const QString &path)
{
    loadSingleScript(path);
}

void LuaScriptEngine::loadSingleScript(const QString &path)
{
    QFileInfo fi(path);
    QString filename = fi.fileName();

    // If already loaded, unload first
    if (m_loadedScripts.contains(filename))
        unloadScript(filename);

    m_currentLoadingScript = filename;

    int err = luaL_dofile(m_L, path.toUtf8().constData());
    if (err != LUA_OK) {
        const char *msg = lua_tostring(m_L, -1);
        qWarning() << "Lua script error in" << filename << ":" << msg;
        emit scriptMessage(QString("Lua error in %1: %2").arg(filename, QString::fromUtf8(msg)));
        lua_pop(m_L, 1);
    } else {
        m_loadedScripts.append(filename);
        qDebug() << "Lua script loaded:" << filename;
    }

    m_currentLoadingScript.clear();
    emit loadedScriptsChanged();
}

void LuaScriptEngine::unloadScript(const QString &filename)
{
    // Remove all hooks registered by this script
    for (int i = m_hooks.size() - 1; i >= 0; --i) {
        if (m_hooks[i].scriptFile == filename) {
            if (m_hooks[i].timer) {
                m_hooks[i].timer->stop();
                delete m_hooks[i].timer;
            }
            luaL_unref(m_L, LUA_REGISTRYINDEX, m_hooks[i].luaRef);
            if (m_hooks[i].userdataRef != LUA_NOREF)
                luaL_unref(m_L, LUA_REGISTRYINDEX, m_hooks[i].userdataRef);
            m_hooks.remove(i);
        }
    }
    m_loadedScripts.removeAll(filename);
    emit loadedScriptsChanged();
}

void LuaScriptEngine::reloadAll()
{
    QStringList scripts = m_loadedScripts;
    for (const QString &s : scripts)
        unloadScript(s);

    QDir dir(m_directory);
    for (const QString &file : dir.entryList({"*.lua"}, QDir::Files))
        loadSingleScript(dir.filePath(file));
}

void LuaScriptEngine::onFileChanged(const QString &path)
{
    QFileInfo fi(path);
    if (fi.suffix() == "lua" && fi.exists()) {
        qDebug() << "Lua script modified, reloading:" << fi.fileName();
        loadSingleScript(path);
    }
}

// ──────────────────────────────────────────────────────────────
//  API implementations
// ──────────────────────────────────────────────────────────────

void LuaScriptEngine::command(const QString &cmd)
{
    if (m_mgr)
        m_mgr->sendMessage("", "/" + cmd);
}

void LuaScriptEngine::prnt(const QString &text)
{
    if (m_mgr) {
        // Route to the active channel's message model
        auto *model = m_mgr->findChild<MessageModel*>();
        if (model)
            model->addMessage("system", text);
    }
}

QString LuaScriptEngine::getInfo(const QString &id)
{
    if (!m_mgr) return {};
    if (id == "nick" || id == "nickname")
        return m_mgr->currentNick();
    if (id == "network" || id == "server" || id == "host") {
        IrcConnection *conn = m_mgr->activeConnection();
        if (conn) return conn->serverHost();
        return {};
    }
    if (id == "channel")
        return m_mgr->property("activeChannel").toString();
    if (id == "topic")
        return m_mgr->channelTopic();
    if (id == "version")
        return QCoreApplication::applicationVersion();
    if (id == "away")
        return {};  // TODO: track away state
    if (id == "event_text")
        return {};
    return {};
}

int LuaScriptEngine::hookCommand(const QString &name, int callbackRef, int userdataRef, int priority)
{
    LuaHook h;
    h.type = LuaHook::Command;
    h.name = name;
    h.luaRef = callbackRef;
    h.userdataRef = userdataRef;
    h.priority = priority;
    h.timer = nullptr;
    h.id = m_nextHookId++;
    h.scriptFile = m_currentLoadingScript;
    m_hooks.append(h);
    return h.id;
}

int LuaScriptEngine::hookServer(const QString &name, int callbackRef, int userdataRef, int priority)
{
    LuaHook h;
    h.type = LuaHook::Server;
    h.name = name;
    h.luaRef = callbackRef;
    h.userdataRef = userdataRef;
    h.priority = priority;
    h.timer = nullptr;
    h.id = m_nextHookId++;
    h.scriptFile = m_currentLoadingScript;
    m_hooks.append(h);
    return h.id;
}

int LuaScriptEngine::hookPrint(const QString &name, int callbackRef, int userdataRef, int priority)
{
    LuaHook h;
    h.type = LuaHook::Print;
    h.name = name;
    h.luaRef = callbackRef;
    h.userdataRef = userdataRef;
    h.priority = priority;
    h.timer = nullptr;
    h.id = m_nextHookId++;
    h.scriptFile = m_currentLoadingScript;
    m_hooks.append(h);
    return h.id;
}

int LuaScriptEngine::hookTimer(int timeout, int callbackRef, int userdataRef)
{
    LuaHook h;
    h.type = LuaHook::Timer;
    h.name = "timer";
    h.luaRef = callbackRef;
    h.userdataRef = userdataRef;
    h.priority = 0;
    h.id = m_nextHookId++;
    h.scriptFile = m_currentLoadingScript;

    QTimer *timer = new QTimer(this);
    timer->setInterval(timeout);
    int hookId = h.id;
    connect(timer, &QTimer::timeout, this, [this, hookId]() {
        for (auto &hook : m_hooks) {
            if (hook.id == hookId && hook.type == LuaHook::Timer) {
                lua_rawgeti(m_L, LUA_REGISTRYINDEX, hook.luaRef);
                if (hook.userdataRef != LUA_NOREF)
                    lua_rawgeti(m_L, LUA_REGISTRYINDEX, hook.userdataRef);
                else
                    lua_pushnil(m_L);

                if (lua_pcall(m_L, 1, 1, 0) != LUA_OK) {
                    qWarning() << "Lua timer callback error:" << lua_tostring(m_L, -1);
                    lua_pop(m_L, 1);
                } else {
                    // If callback returns 0 or false, remove the timer
                    int keep = 1;
                    if (lua_isboolean(m_L, -1))
                        keep = lua_toboolean(m_L, -1);
                    else if (lua_isinteger(m_L, -1))
                        keep = (int)lua_tointeger(m_L, -1);
                    lua_pop(m_L, 1);
                    if (!keep) {
                        unhook(hookId);
                        return;
                    }
                }
                break;
            }
        }
    });
    timer->start();
    h.timer = timer;
    m_hooks.append(h);
    return h.id;
}

void LuaScriptEngine::unhook(int hookId)
{
    for (int i = 0; i < m_hooks.size(); ++i) {
        if (m_hooks[i].id == hookId) {
            if (m_hooks[i].timer) {
                m_hooks[i].timer->stop();
                delete m_hooks[i].timer;
            }
            luaL_unref(m_L, LUA_REGISTRYINDEX, m_hooks[i].luaRef);
            if (m_hooks[i].userdataRef != LUA_NOREF)
                luaL_unref(m_L, LUA_REGISTRYINDEX, m_hooks[i].userdataRef);
            m_hooks.remove(i);
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────
//  Command / server line dispatch
// ──────────────────────────────────────────────────────────────

bool LuaScriptEngine::handleCommand(const QString &command, const QStringList &args)
{
    if (!m_L) return false;

    QString cmdUpper = command.toUpper();
    for (auto &hook : m_hooks) {
        if (hook.type != LuaHook::Command || hook.name != cmdUpper)
            continue;

        lua_rawgeti(m_L, LUA_REGISTRYINDEX, hook.luaRef);

        // Build word table (1-indexed)
        lua_newtable(m_L);
        for (int i = 0; i < args.size(); ++i) {
            lua_pushstring(m_L, args[i].toUtf8().constData());
            lua_rawseti(m_L, -2, i + 1);
        }

        // Build word_eol table (1-indexed)
        lua_newtable(m_L);
        for (int i = 0; i < args.size(); ++i) {
            QStringList tail = args.mid(i);
            lua_pushstring(m_L, tail.join(' ').toUtf8().constData());
            lua_rawseti(m_L, -2, i + 1);
        }

        // Userdata or nil
        if (hook.userdataRef != LUA_NOREF)
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, hook.userdataRef);
        else
            lua_pushnil(m_L);

        if (lua_pcall(m_L, 3, 1, 0) != LUA_OK) {
            qWarning() << "Lua command hook error:" << lua_tostring(m_L, -1);
            lua_pop(m_L, 1);
            continue;
        }

        int ret = EAT_NONE;
        if (lua_isinteger(m_L, -1))
            ret = (int)lua_tointeger(m_L, -1);
        lua_pop(m_L, 1);

        if (ret >= EAT_HEXCHAT)
            return true;
    }
    return false;
}

bool LuaScriptEngine::handleServerLine(IrcConnection * /*conn*/, const QString &rawLine)
{
    if (!m_L) return false;

    // Parse the numeric/command from the raw line
    QStringList parts = rawLine.split(' ');
    if (parts.size() < 2) return false;
    QString cmd = parts[1].toUpper();

    for (auto &hook : m_hooks) {
        if (hook.type != LuaHook::Server)
            continue;
        if (hook.name != cmd && hook.name != "RAW LINE")
            continue;

        lua_rawgeti(m_L, LUA_REGISTRYINDEX, hook.luaRef);

        // Build word table
        lua_newtable(m_L);
        for (int i = 0; i < parts.size(); ++i) {
            lua_pushstring(m_L, parts[i].toUtf8().constData());
            lua_rawseti(m_L, -2, i + 1);
        }

        // Build word_eol table
        lua_newtable(m_L);
        for (int i = 0; i < parts.size(); ++i) {
            QStringList tail = parts.mid(i);
            lua_pushstring(m_L, tail.join(' ').toUtf8().constData());
            lua_rawseti(m_L, -2, i + 1);
        }

        if (hook.userdataRef != LUA_NOREF)
            lua_rawgeti(m_L, LUA_REGISTRYINDEX, hook.userdataRef);
        else
            lua_pushnil(m_L);

        if (lua_pcall(m_L, 3, 1, 0) != LUA_OK) {
            qWarning() << "Lua server hook error:" << lua_tostring(m_L, -1);
            lua_pop(m_L, 1);
            continue;
        }

        int ret = EAT_NONE;
        if (lua_isinteger(m_L, -1))
            ret = (int)lua_tointeger(m_L, -1);
        lua_pop(m_L, 1);

        if (ret >= EAT_HEXCHAT)
            return true;
    }
    return false;
}
