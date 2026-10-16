#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QStringList>

#if __has_include(<QJSEngine>)
#include <QJSEngine>
#define HAVE_QJSE 1
#else
#define HAVE_QJSE 0
#endif

class IrcConnection;
class ScriptManagerTestAccess;

class ScriptManager : public QObject
{
    Q_OBJECT
public:
    explicit ScriptManager(QObject *parent = nullptr);
    ~ScriptManager();

    // Directories that may contain *.js, skipping any that do not exist.
    static QStringList candidateDirectories();

    void loadScripts(const QString &directory);
    void loadScripts(const QStringList &directories);

    // Raw IRC line. PRIVMSG/NOTICE bodies are passed to the JS onMessage
    // function as (connection, nick, text). Other lines are ignored.
    void handleRawServerLine(IrcConnection *conn, const QString &rawLine);

    void handleMessage(IrcConnection *conn, const QString &sender, const QString &message);

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);

private:
    void loadFile(const QString &path);
    friend class ScriptManagerTestAccess;

#if HAVE_QJSE
    QJSEngine m_engine;
#endif
    QFileSystemWatcher *m_watcher = nullptr;
    QStringList m_directories;
    QStringList m_loadedFiles;
};
