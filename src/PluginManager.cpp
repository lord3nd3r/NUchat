#include "PluginManager.h"
#include "IrcLineParse.h"
#include "PluginInterface.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QSet>
#include <QStandardPaths>
#include <utility>

PluginManager *PluginManager::s_instance = nullptr;

PluginManager::PluginManager(QObject *parent) : QObject(parent) {
    s_instance = this;
}

PluginManager::~PluginManager() {
    // Instances are owned by their loaders only after unload(); until then
    // we own them. Delete the objects while the libraries are still mapped,
    // then drop the loaders.
    qDeleteAll(plugins);
    plugins.clear();
    qDeleteAll(loaders);
    loaders.clear();
    if (s_instance == this)
        s_instance = nullptr;
}

PluginManager *PluginManager::instance() { return s_instance; }

static QString expandTilde(QString path) {
    if (path == QLatin1String("~"))
        return QDir::homePath();
    if (path.startsWith(QLatin1String("~/")) || path.startsWith(QLatin1String("~\\")))
        return QDir::homePath() + path.mid(1);
    return path;
}

static void addExistingDir(QStringList &dirs, const QString &path) {
    if (path.isEmpty())
        return;
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir())
        return;
    const QString abs = info.absoluteFilePath();
    if (!dirs.contains(abs))
        dirs.append(abs);
}

QStringList PluginManager::candidateDirectories(const QString &preferred) {
    QStringList dirs;
    addExistingDir(dirs, expandTilde(preferred.trimmed()));

    const QString appDir = QCoreApplication::applicationDirPath();
    // Portable layout and Windows installs: plugins beside the executable.
    addExistingDir(dirs, appDir + QStringLiteral("/plugins"));
    // In-tree build: executable is <build>/src/nuchat, plugins land in
    // <build>/plugins. Only when this is actually a build tree — an
    // installed /usr/bin/nuchat must not scan /usr/plugins.
    if (QFileInfo::exists(appDir + QStringLiteral("/../CMakeCache.txt")))
        addExistingDir(dirs, appDir + QStringLiteral("/../plugins"));

#ifdef NUCHAT_PLUGIN_INSTALL_DIR
    addExistingDir(dirs, QStringLiteral(NUCHAT_PLUGIN_INSTALL_DIR));
#endif
    // Relocated installs: <prefix>/bin/nuchat → <prefix>/lib[/triplet]/nuchat/plugins
    // and a flat prefix where the exe sits next to lib/.
    addExistingDir(dirs, appDir + QStringLiteral("/lib/nuchat/plugins"));
    addExistingDir(dirs, appDir + QStringLiteral("/../lib/nuchat/plugins"));
    const QDir libDir(appDir + QStringLiteral("/../lib"));
    if (libDir.exists()) {
        const auto triplets = libDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &sub : triplets)
            addExistingDir(dirs, libDir.absoluteFilePath(sub + QStringLiteral("/nuchat/plugins")));
    }
    const QDir libDirFlat(appDir + QStringLiteral("/lib"));
    if (libDirFlat.exists()) {
        const auto triplets = libDirFlat.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &sub : triplets)
            addExistingDir(dirs, libDirFlat.absoluteFilePath(sub + QStringLiteral("/nuchat/plugins")));
    }

    addExistingDir(dirs,
                   QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
                       QStringLiteral("/NUchat/plugins"));
    return dirs;
}

static const QStringList kPluginExtensions =
#if defined(Q_OS_WIN)
    {"*.dll"};
#elif defined(Q_OS_MACOS)
    {"*.dylib"};
#else
    {"*.so"};
#endif

void PluginManager::loadPlugins(const QString &directory) {
    loadPlugins(QStringList{directory});
}

void PluginManager::loadPlugins(const QStringList &directories) {
    QSet<QString> seen;
    int loaded = 0;
    for (const QString &directory : directories) {
        QDir dir(directory);
        if (!dir.exists())
            continue;
        const QStringList files = dir.entryList(kPluginExtensions, QDir::Files);
        for (const QString &file : files) {
            const QString filePath = dir.absoluteFilePath(file);
            QString canon = QFileInfo(filePath).canonicalFilePath();
            if (canon.isEmpty())
                canon = QFileInfo(filePath).absoluteFilePath();
            if (seen.contains(canon))
                continue;
            seen.insert(canon);

            // Not parented: ~QPluginLoader does not unload, so the instance
            // stays valid until we delete it below. Parenting plus an
            // explicit delete would be easy to get wrong at shutdown.
            auto *loader = new QPluginLoader(filePath);
            const QString iid = loader->metaData().value(QStringLiteral("IID")).toString();
            if (iid != QLatin1String(PluginInterface_iid)) {
                qWarning() << "[Plugins] Skipping" << file
                           << "- IID mismatch (got" << iid << ")";
                delete loader;
                continue;
            }

            QObject *plugin = loader->instance();
            if (!plugin) {
                qWarning() << "[Plugins] Failed to load" << file << ":"
                           << loader->errorString();
                delete loader;
                continue;
            }

            auto *pi = qobject_cast<PluginInterface *>(plugin);
            if (!pi) {
                qWarning() << "[Plugins] Skipping" << file
                           << "- does not implement PluginInterface";
                loader->unload();
                delete loader;
                continue;
            }

            pi->initialize(this);
            plugins.append(pi);
            loaders.append(loader);
            ++loaded;
            qDebug() << "[Plugins] Loaded" << file << "from" << directory;
        }
    }
    qDebug() << "[Plugins] Loaded" << loaded << "plugin(s)";
}

bool PluginManager::handleUserCommand(const QString &command, const QStringList &args,
                                      IrcConnection *conn) {
    const QString cmd = command.trimmed().toLower();
    if (cmd.isEmpty())
        return false;
    for (PluginInterface *pi : std::as_const(plugins)) {
        if (pi->handleCommand(cmd, args, conn))
            return true;
    }
    return false;
}

void PluginManager::onMessage(IrcConnection *conn, const QString &msg) {
    const BangCommand bang = bangCommandFromRaw(msg);
    if (bang.command.isEmpty())
        return;
    handleUserCommand(bang.command, bang.args, conn);
}
