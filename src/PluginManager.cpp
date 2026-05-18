#include "PluginManager.h"
#include "PluginInterface.h"

#include <QDir>
#include <QJsonObject>
#include <QPluginLoader>
#include <utility>

PluginManager::PluginManager(QObject *parent) : QObject(parent) {}

PluginManager::~PluginManager() { qDeleteAll(plugins); }

static const QStringList kPluginExtensions =
#if defined(Q_OS_WIN)
    {"*.dll"};
#elif defined(Q_OS_MACOS)
    {"*.dylib"};
#else
    {"*.so"};
#endif

void PluginManager::loadPlugins(const QString &directory) {
  QDir dir(directory);
  const QStringList files =
      dir.entryList(kPluginExtensions, QDir::Files);
  for (const QString &file : files) {
    const QString filePath = dir.absoluteFilePath(file);
    QPluginLoader loader(filePath);

    // Validate the embedded IID before loading any code.
    const QJsonObject meta = loader.metaData().value("MetaData").toObject();
    const QString iid = loader.metaData().value("IID").toString();
    if (iid != QLatin1String(PluginInterface_iid)) {
      qWarning() << "[Plugins] Skipping" << file
                 << "- IID mismatch (got" << iid << ")";
      continue;
    }

    QObject *plugin = loader.instance();
    if (!plugin) {
      qWarning() << "[Plugins] Failed to load" << file << ":" << loader.errorString();
      continue;
    }

    auto pi = qobject_cast<PluginInterface *>(plugin);
    if (!pi) {
      qWarning() << "[Plugins] Skipping" << file << "- does not implement PluginInterface";
      loader.unload();
      continue;
    }

    pi->initialize(this);
    plugins.append(pi);
    qDebug() << "[Plugins] Loaded" << file;
  }
}

void PluginManager::onMessage(IrcConnection *conn, const QString &msg) {
  // simple command parsing: messages starting with '!'
  if (!msg.startsWith("!"))
    return;
  QStringList parts = msg.mid(1).split(' ');
  QString cmd = parts.takeFirst();
  for (PluginInterface *pi : std::as_const(plugins)) {
    if (pi->handleCommand(cmd, parts, conn))
      break;
  }
}
