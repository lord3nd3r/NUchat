#include "ScriptManager.h"
#include "IrcConnection.h"
#include "IrcLineParse.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#if !HAVE_QJSE
// stub implementations when QJSEngine is unavailable
#endif

ScriptManager::ScriptManager(QObject *parent) : QObject(parent) {
#if HAVE_QJSE
  // Qt 6 injects console.log into QJSEngine automatically.
#endif
}

ScriptManager::~ScriptManager() {
  // nothing special; avoid tearing down Qt objects at exit
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

QStringList ScriptManager::candidateDirectories() {
    QStringList dirs;
#ifdef NUCHAT_SOURCE_SCRIPTS_DIR
    // Present for in-tree runs. Absent (and therefore skipped) after install
    // on a machine that does not have the source tree.
    addExistingDir(dirs, QStringLiteral(NUCHAT_SOURCE_SCRIPTS_DIR));
#endif
    const QString appDir = QCoreApplication::applicationDirPath();
    addExistingDir(dirs, appDir + QStringLiteral("/scripts"));
    addExistingDir(dirs, appDir + QStringLiteral("/../scripts"));
#ifdef NUCHAT_INSTALL_SCRIPTS_DIR
    addExistingDir(dirs, QStringLiteral(NUCHAT_INSTALL_SCRIPTS_DIR));
#endif
    // <prefix>/bin/nuchat → <prefix>/share/nuchat/scripts
    addExistingDir(dirs, appDir + QStringLiteral("/../share/nuchat/scripts"));
    addExistingDir(dirs,
                   QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
                       QStringLiteral("/NUchat/scripts"));
    return dirs;
}

void ScriptManager::loadScripts(const QString &directory) {
    loadScripts(QStringList{directory});
}

void ScriptManager::loadScripts(const QStringList &directories) {
#if HAVE_QJSE
    if (!m_watcher) {
        m_watcher = new QFileSystemWatcher(this);
        connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
                &ScriptManager::onFileChanged);
        connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
                &ScriptManager::onDirectoryChanged);
    }
    for (const QString &directory : directories) {
        QDir dir(directory);
        if (!dir.exists())
            continue;
        const QString abs = dir.absolutePath();
        if (!m_directories.contains(abs))
            m_directories.append(abs);
        m_watcher->addPath(abs);
        for (const QString &file : dir.entryList({QStringLiteral("*.js")}, QDir::Files))
            loadFile(dir.filePath(file));
    }
#else
    Q_UNUSED(directories);
#endif
}

void ScriptManager::loadFile(const QString &path) {
#if HAVE_QJSE
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QString code = QString::fromUtf8(f.readAll());
    const QJSValue result = m_engine.evaluate(code, path);
    if (result.isError()) {
        qWarning() << "[ScriptManager] Error in" << path << ":" << result.toString();
    }
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (!m_loadedFiles.contains(abs))
        m_loadedFiles.append(abs);
    if (m_watcher && !m_watcher->files().contains(abs))
        m_watcher->addPath(abs);
#else
    Q_UNUSED(path);
#endif
}

void ScriptManager::handleRawServerLine(IrcConnection *conn, const QString &rawLine) {
    const IrcLineView view = parseIrcLine(rawLine);
    if (view.command != QLatin1String("PRIVMSG") &&
        view.command != QLatin1String("NOTICE"))
        return;
    handleMessage(conn, ircNickFromPrefix(view.prefix), ircTrailing(rawLine));
}

void ScriptManager::handleMessage(IrcConnection *conn, const QString &sender,
                                  const QString &message) {
#if HAVE_QJSE
  QJSValue func = m_engine.globalObject().property("onMessage");
  if (func.isCallable()) {
    QJSValueList args;
    args << m_engine.newQObject(conn);
    args << sender;
    args << message;
    const QJSValue result = func.call(args);
    if (result.isError())
        qWarning() << "[ScriptManager] onMessage error:" << result.toString();
  }
#else
  Q_UNUSED(conn)
  Q_UNUSED(sender)
  Q_UNUSED(message)
#endif
}

void ScriptManager::onFileChanged(const QString &path) {
#if HAVE_QJSE
  if (!path.endsWith(QLatin1String(".js")))
      return;
  loadFile(path);
  if (m_watcher && !m_watcher->files().contains(path))
      m_watcher->addPath(path);
#else
  Q_UNUSED(path)
#endif
}

void ScriptManager::onDirectoryChanged(const QString &path) {
#if HAVE_QJSE
    QDir dir(path);
    if (!dir.exists())
        return;
    for (const QString &file : dir.entryList({QStringLiteral("*.js")}, QDir::Files)) {
        const QString full = dir.absoluteFilePath(file);
        if (!m_loadedFiles.contains(full))
            loadFile(full);
    }
#else
    Q_UNUSED(path);
#endif
}
