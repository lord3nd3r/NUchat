#include "ScriptManager.h"
#include "IrcConnection.h"
#include <QDebug>
#include <QDir>
#include <QFile>

#if !HAVE_QJSE
// stub implementations when QJSEngine is unavailable
#endif

ScriptManager::ScriptManager(QObject *parent) : QObject(parent) {
#if HAVE_QJSE
  // expose simple API to scripts
  QJSValue global = m_engine.globalObject();
  // In Qt6, print/console is generally accessible or evaluating isn't needed
  // here. Qt automatically injects console.log
#endif
}

ScriptManager::~ScriptManager() {
  // nothing special; avoid tearing down Qt objects at exit
}

void ScriptManager::loadScripts(const QString &directory) {
#if HAVE_QJSE
  m_directory = directory;
  QDir dir(directory);
  QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
  watcher->addPath(directory);
  connect(watcher, &QFileSystemWatcher::fileChanged, this,
          &ScriptManager::onFileChanged);

  for (const QString &file : dir.entryList({"*.js"}, QDir::Files)) {
    QFile f(dir.filePath(file));
    if (f.open(QIODevice::ReadOnly)) {
      QString code = f.readAll();
      m_engine.evaluate(code, file);
    }
  }
#endif
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
    func.call(args);
  }
#else
  Q_UNUSED(conn)
  Q_UNUSED(sender)
  Q_UNUSED(message)
#endif
}

void ScriptManager::onFileChanged(const QString &path) {
#if HAVE_QJSE
  // reload changed file
  QFile f(path);
  if (f.open(QIODevice::ReadOnly)) {
    QString code = f.readAll();
    m_engine.evaluate(code, path);
  }
#else
  Q_UNUSED(path)
#endif
}
