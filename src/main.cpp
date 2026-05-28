#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QSize>
#include <QStandardPaths>
#include <QWindow>
#ifdef QT_QUICK_LIB
#include <QQmlApplicationEngine>
#include <QQmlContext>
#endif
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "DccManager.h"
#include "IRCConnectionManager.h"
#include "ImageDownloader.h"
#include "IrcConnection.h"
#include "Logger.h"
#include "MessageModel.h"
#include "NotificationManager.h"
#include "PluginManager.h"
#include "ScriptManager.h"
#include "ServerChannelModel.h"
#include "Settings.h"
#include "ThemeManager.h"
#include "Version.h"
#ifdef HAVE_PYTHON
#include "PythonScriptEngine.h"
#endif
#ifdef HAVE_LUA
#include "LuaScriptEngine.h"
#endif
#ifdef HAVE_HUNSPELL
#include "SpellChecker.h"
#include "SpellHighlighter.h"
#endif

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setOrganizationName("NUchat");
  app.setApplicationName("NUchat");
  app.setApplicationVersion(NUCHAT_VERSION);
  app.setDesktopFileName("nuchat");
  
  // Set application icon with multiple sizes for better cross-platform support
  QIcon appIcon;
  appIcon.addFile(":/icons/nuchat-16.png", QSize(16, 16));
  appIcon.addFile(":/icons/nuchat-32.png", QSize(32, 32));
  appIcon.addFile(":/icons/nuchat-48.png", QSize(48, 48));
  appIcon.addFile(":/icons/nuchat-64.png", QSize(64, 64));
  appIcon.addFile(":/icons/nuchat-128.png", QSize(128, 128));
  appIcon.addFile(":/icons/nuchat-256.png", QSize(256, 256));
  appIcon.addFile(":/icons/nuchat.svg"); // Fallback to SVG for scalability
  app.setWindowIcon(appIcon);

  // register types
#ifdef QT_QUICK_LIB
  qmlRegisterSingletonType<Settings>(
      "NUchat", 1, 0, "Settings",
      [](QQmlEngine *, QJSEngine *) -> QObject * { return new Settings(); });
  qmlRegisterType<ThemeManager>("NUchat", 1, 0, "ThemeManager");
  qmlRegisterType<ServerChannelModel>("NUchat", 1, 0, "ServerChannelModel");
  qmlRegisterType<MessageModel>("NUchat", 1, 0, "MessageModel");
#ifdef HAVE_HUNSPELL
  qmlRegisterType<SpellHighlighter>("NUchat", 1, 0, "SpellHighlighter");
  qmlRegisterType<SpellChecker>("NUchat", 1, 0, "SpellChecker");
#endif
#endif

  IRCConnectionManager manager;
  Logger logger;
  ThemeManager themeManager;
  ServerChannelModel treeModel;
  MessageModel msgModel;
  Settings appSettings;
  // Scripting engines must be destroyed before the manager, but after the
  // event loop ends.  Using QObject parent=&manager would crash because
  // QObject children are destroyed in reverse order and the engines
  // reference manager internals.  Instead, destroy them explicitly on
  // aboutToQuit (which fires before stack-allocated objects unwind).
  ScriptManager *scriptMgr = new ScriptManager(&manager);
  PluginManager pluginMgr;
#ifdef HAVE_PYTHON
  PythonScriptEngine *pyEngine = new PythonScriptEngine(&manager, nullptr);
#endif
#ifdef HAVE_LUA
  LuaScriptEngine *luaEngine = new LuaScriptEngine(&manager, nullptr);
#endif

  QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
#ifdef HAVE_LUA
    delete luaEngine;
    luaEngine = nullptr;
#endif
#ifdef HAVE_PYTHON
    delete pyEngine;
    pyEngine = nullptr;
#endif
    delete scriptMgr;
    scriptMgr = nullptr;
  });

  // Wire the manager to our models
  manager.setMessageModel(&msgModel);
  manager.setServerChannelModel(&treeModel);
  manager.setLogger(&logger);
  manager.setSettings(&appSettings);

  // ── System tray and notifications ──
  NotificationManager notifyMgr;
  notifyMgr.setSettings(&appSettings);

  // Wire notifications: highlights and PMs trigger desktop notifications + sounds
  QObject::connect(
      &manager, &IRCConnectionManager::notifyUser,
      [&notifyMgr](const QString &title, const QString &message,
                   bool isHighlight, bool isPrivate) {
        notifyMgr.notify(title, message, isHighlight, isPrivate);
        notifyMgr.playSound(isHighlight, isPrivate);
      });

  // Update tray icon unread state when unread changes
  QObject::connect(&manager, &IRCConnectionManager::unreadStateChanged,
                   [&manager, &notifyMgr]() {
    // Check if any channels have unread or highlights
    // We use a simple approach: if there's any unread state signal, at least one is unread
    Q_UNUSED(manager);
    // The unreadStateChanged signal means something changed — the QML side
    // manages the actual per-channel state, so just toggle the icon.
    // For now, set highlight state since that's when we notify.
    notifyMgr.setUnreadState(true, true);
  });

  // Quit from tray context menu
  QObject::connect(&notifyMgr, &NotificationManager::quitRequested,
                   &app, &QApplication::quit);

#ifdef QT_QUICK_LIB
  QQmlApplicationEngine engine;
  // ensure resources compiled into static library are registered
  Q_INIT_RESOURCE(resources);
#endif

  // try to load user scripts from workspace/scripts
  scriptMgr->loadScripts(QCoreApplication::applicationDirPath() +
                         "/../scripts");

#ifdef HAVE_PYTHON
  // Load Python scripts from ~/.config/NUchat/scripts/
  QString pyScriptsDir =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
      "/NUchat/scripts";
  pyEngine->loadScripts(pyScriptsDir);
#endif

#ifdef HAVE_LUA
  // Load Lua scripts from ~/.config/NUchat/scripts/
  QString luaScriptsDir =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) +
      "/NUchat/scripts";
  luaEngine->loadScripts(luaScriptsDir);
#endif

#ifdef QT_QUICK_LIB
  engine.rootContext()->setContextProperty("ircManager", &manager);
  engine.rootContext()->setContextProperty("themeManager", &themeManager);
  engine.rootContext()->setContextProperty("treeModel", &treeModel);
  engine.rootContext()->setContextProperty("msgModel", &msgModel);
  engine.rootContext()->setContextProperty("scriptMgr", scriptMgr);
  engine.rootContext()->setContextProperty("pluginMgr", &pluginMgr);
#ifdef HAVE_PYTHON
  engine.rootContext()->setContextProperty("pyEngine", pyEngine);
#endif
#ifdef HAVE_LUA
  engine.rootContext()->setContextProperty("luaEngine", luaEngine);
#endif
  engine.rootContext()->setContextProperty("appSettings", &appSettings);
  engine.rootContext()->setContextProperty("notifyMgr", &notifyMgr);
  engine.rootContext()->setContextProperty("imgDownloader",
                                           ImageDownloader::instance());
  // DCC file transfer manager
  DccManager dccManager;
  QString dccDownDir = appSettings.value("dcc/downloadDir", QDir::homePath() + "/Downloads").toString();
  dccManager.setDownloadDir(dccDownDir);
  dccManager.setAutoAccept(appSettings.value("dcc/autoAccept", false).toBool());
  qint64 maxDcc = appSettings.value("dcc/maxFileSize", 100 * 1024 * 1024).toLongLong();
  dccManager.setMaxFileSize(maxDcc);
  manager.setDccManager(&dccManager);
  engine.rootContext()->setContextProperty("dccManager", &dccManager);
  engine.rootContext()->setContextProperty("appVersion",
                                           QString(NUCHAT_VERSION));
#endif

  // load plugins from build/plugins (or install directory)
  pluginMgr.loadPlugins(QCoreApplication::applicationDirPath() + "/../plugins");

  // when a new connection is added, hook it to plugin and script managers
  QObject::connect(
      &manager, &IRCConnectionManager::clientAdded, [&](IrcConnection *conn) {
        QObject::connect(conn, &IrcConnection::rawLineReceived,
                         [&](const QString &msg) {
                           pluginMgr.onMessage(conn, msg);
                           scriptMgr->handleMessage(conn, QString(), msg);
#ifdef HAVE_PYTHON
                           pyEngine->handleServerLine(conn, msg);
#endif
#ifdef HAVE_LUA
                           luaEngine->handleServerLine(conn, msg);
#endif
                         });
      });

#ifdef QT_QUICK_LIB
  const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
#ifdef Q_OS_WIN
          // WIN32 subsystem has no console — surface the error as a native dialog
          // so the user isn't left with a silently-disappearing process.
          MessageBoxW(
              nullptr,
              L"NUchat could not start because the user interface failed to load.\n\n"
              L"Qt Quick libraries may be missing from the installation directory.\n\n"
              L"Please reinstall NUchat or report this issue.",
              L"NUchat \u2014 Fatal Error",
              MB_ICONERROR | MB_OK);
#endif
          QCoreApplication::exit(-1);
        }
      },
      Qt::QueuedConnection);
  engine.load(url);

  // Pass the top-level window to the notification manager for show/hide
  if (!engine.rootObjects().isEmpty()) {
    QWindow *win = qobject_cast<QWindow *>(engine.rootObjects().first());
    if (win)
      notifyMgr.setWindow(win);
  }
#endif

  // load default theme
  themeManager.loadTheme(":/themes/default.json");

  // Welcome message — no sample data; real data comes from IRC
  msgModel.addMessage("system",
                      "Welcome to NUchat — an open-source IRC client.");
  msgModel.addMessage(
      "system", "Use NUchat → Quick Connect or /join #channel to get started.");

  qDebug() << "starting event loop";
  int res = app.exec();
  qDebug() << "event loop exited" << res;
  return res;
}
