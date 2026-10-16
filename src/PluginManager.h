#pragma once

#include <QObject>
#include <QStringList>
#include <QVector>

class PluginInterface;
class IrcConnection;
class QPluginLoader;

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    static PluginManager *instance();

    // Directories searched for plugins, in order. `preferred` is the
    // plugin/directory setting (tilde-expanded); empty skips that entry.
    // Missing directories are omitted.
    static QStringList candidateDirectories(const QString &preferred = QString());

    void loadPlugins(const QString &directory);
    void loadPlugins(const QStringList &directories);

    int loadedCount() const { return plugins.size(); }

    // User-typed "/command". `command` is matched case-insensitively.
    // Returns true when a plugin consumed it.
    bool handleUserCommand(const QString &command, const QStringList &args,
                           IrcConnection *conn);

public slots:
    // Raw server line. Dispatches "!command" from PRIVMSG/NOTICE text.
    void onMessage(IrcConnection *conn, const QString &msg);

private:
    QVector<PluginInterface*> plugins;
    QVector<QPluginLoader*> loaders;
    static PluginManager *s_instance;
};
