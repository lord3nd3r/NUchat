#include "PluginManager.h"
#include "PluginInterface.h"

#include <QDir>
#include <QPluginLoader>

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
}

PluginManager::~PluginManager()
{
    qDeleteAll(plugins);
}

void PluginManager::loadPlugins(const QString &directory)
{
    QDir dir(directory);
    for (const QString &file : dir.entryList(QDir::Files)) {
        QPluginLoader loader(dir.absoluteFilePath(file));
        QObject *plugin = loader.instance();
        if (plugin) {
            auto pi = qobject_cast<PluginInterface*>(plugin);
            if (pi) {
                pi->initialize(this);
                plugins.append(pi);
            }
        }
    }
}

void PluginManager::onMessage(IrcConnection *conn, const QString &msg)
{
    // simple command parsing: messages starting with '!'
    if (!msg.startsWith("!"))
        return;
    QStringList parts = msg.mid(1).split(' ');
    QString cmd = parts.takeFirst();
    for (PluginInterface *pi : qAsConst(plugins)) {
        if (pi->handleCommand(cmd, parts, conn))
            break;
    }
}
