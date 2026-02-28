#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QFileSystemWatcher>

// Forward-declare lua_State to avoid exposing Lua headers
struct lua_State;

class IRCConnectionManager;
class IrcConnection;

struct LuaHook {
    enum Type { Command, Server, Print, Timer };
    Type type;
    QString name;       // command name / server numeric / print event
    int luaRef;         // Lua registry ref for the callback function
    int userdataRef;    // Lua registry ref for userdata (LUA_NOREF if none)
    int priority;
    QTimer *timer;      // only for Timer hooks
    int id;
    QString scriptFile; // which script registered this hook
};

class LuaScriptEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList loadedScripts READ loadedScripts NOTIFY loadedScriptsChanged)
public:
    explicit LuaScriptEngine(IRCConnectionManager *mgr, QObject *parent = nullptr);
    ~LuaScriptEngine();

    void loadScripts(const QString &directory);

    Q_INVOKABLE QStringList loadedScripts() const { return m_loadedScripts; }
    Q_INVOKABLE void loadScript(const QString &path);
    Q_INVOKABLE void unloadScript(const QString &filename);
    Q_INVOKABLE void reloadAll();

    // Called by IRCConnectionManager
    bool handleCommand(const QString &command, const QStringList &args);
    bool handleServerLine(IrcConnection *conn, const QString &rawLine);

    // ── API called from Lua C functions ──
    void command(const QString &cmd);
    void prnt(const QString &text);
    QString getInfo(const QString &id);
    int hookCommand(const QString &name, int callbackRef, int userdataRef, int priority);
    int hookServer(const QString &name, int callbackRef, int userdataRef, int priority);
    int hookPrint(const QString &name, int callbackRef, int userdataRef, int priority);
    int hookTimer(int timeout, int callbackRef, int userdataRef);
    void unhook(int hookId);

    lua_State *luaState() const { return m_L; }
    static LuaScriptEngine *instance();

signals:
    void loadedScriptsChanged();
    void scriptMessage(const QString &msg);

private slots:
    void onFileChanged(const QString &path);

private:
    void initLua();
    void shutdownLua();
    void registerAPI();
    void loadSingleScript(const QString &path);

    IRCConnectionManager *m_mgr;
    lua_State *m_L = nullptr;
    QString m_directory;
    QFileSystemWatcher *m_watcher = nullptr;
    QVector<LuaHook> m_hooks;
    QStringList m_loadedScripts;
    QString m_currentLoadingScript;  // track which script is being loaded
    int m_nextHookId = 1;

    static LuaScriptEngine *s_instance;
};
