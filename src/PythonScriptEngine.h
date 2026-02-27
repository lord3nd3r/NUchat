#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QFileSystemWatcher>

class IRCConnectionManager;
class IrcConnection;

// ── Embedded Python engine providing HexChat-compatible API ──
// Scripts are loaded from ~/.config/NUchat/scripts/
// The engine exposes a `hexchat` module with the standard HexChat Python API.

struct PyHook {
    enum Type { Command, Server, Print, Timer };
    Type type;
    QString name;        // command name / server numeric / print event
    void *pyCallback;    // borrowed PyObject* (callable)
    void *pyUserdata;    // borrowed PyObject* or NULL
    int priority;
    QTimer *timer;       // only for Timer hooks
    int id;
};

class PythonScriptEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList loadedScripts READ loadedScripts NOTIFY loadedScriptsChanged)
    Q_PROPERTY(QString scriptsDirectory READ scriptsDirectory CONSTANT)
public:
    explicit PythonScriptEngine(IRCConnectionManager *mgr, QObject *parent = nullptr);
    ~PythonScriptEngine();

    // Load all *.py from directory, set up file watcher
    void loadScripts(const QString &directory);

    // QML-accessible methods
    Q_INVOKABLE QStringList loadedScripts() const { return m_loadedScripts; }
    Q_INVOKABLE QString scriptsDirectory() const { return m_directory; }
    Q_INVOKABLE void loadScript(const QString &path);
    Q_INVOKABLE void unloadScript(const QString &filename);
    Q_INVOKABLE void reloadScript(const QString &filename);
    Q_INVOKABLE void reloadAll();
    Q_INVOKABLE void openScriptsFolder();
    Q_INVOKABLE QString scriptInfo(const QString &filename) const;
    Q_INVOKABLE int hookCount() const { return m_hooks.size(); }

    // Called by IRCConnectionManager when raw lines arrive
    bool handleServerLine(IrcConnection *conn, const QString &rawLine);

    // Called when the user types a command (returns true if a hook consumed it)
    bool handleCommand(const QString &command, const QStringList &args);

    // Called for print events
    bool handlePrintEvent(const QString &event, const QStringList &args);

    // ── HexChat API (called from Python C functions) ──
    void command(const QString &cmd);
    void prnt(const QString &text);
    QString getInfo(const QString &id);
    int hookCommand(const QString &name, void *callback, void *userdata, int priority);
    int hookServer(const QString &name, void *callback, void *userdata, int priority);
    int hookPrint(const QString &name, void *callback, void *userdata, int priority);
    int hookTimer(int timeout, void *callback, void *userdata);
    void unhook(int hookId);

    static PythonScriptEngine *instance();

signals:
    void loadedScriptsChanged();
    void scriptMessage(const QString &msg);

private slots:
    void onFileChanged(const QString &path);

private:
    void initPython();
    void shutdownPython();
    void loadSingleScript(const QString &path);

    IRCConnectionManager *m_mgr;
    QString m_directory;
    QFileSystemWatcher *m_watcher = nullptr;
    QVector<PyHook> m_hooks;
    QStringList m_loadedScripts;               // filenames of loaded scripts
    QMap<QString, QStringList> m_scriptHooks;  // filename -> list of hook IDs
    QMap<QString, QString> m_scriptInfo;       // filename -> __module_name__ + version + desc
    int m_nextHookId = 1;
    bool m_pyInited = false;

    static PythonScriptEngine *s_instance;
};
