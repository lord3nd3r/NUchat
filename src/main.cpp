#include <QGuiApplication>
#include <QIcon>
#include <QStandardPaths>
#ifdef QT_QUICK_LIB
#include <QQmlApplicationEngine>
#include <QQmlContext>
#endif

#include "IRCConnectionManager.h"
#include "ImageDownloader.h"
#include "IrcConnection.h"
#include "Logger.h"
#include "MessageModel.h"
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
  QGuiApplication app(argc, argv);
  app.setOrganizationName("NUchat");
  app.setApplicationName("NUchat");
  app.setApplicationVersion(NUCHAT_VERSION);
  app.setDesktopFileName("nuchat");
  app.setWindowIcon(QIcon(":/icons/nuchat.svg"));

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
  // allocate script manager on heap to avoid destructor crash
  ScriptManager *scriptMgr = new ScriptManager(&manager);
  PluginManager pluginMgr;
#ifdef HAVE_PYTHON
  PythonScriptEngine *pyEngine = new PythonScriptEngine(&manager, &manager);
#endif
#ifdef HAVE_LUA
  LuaScriptEngine *luaEngine = new LuaScriptEngine(&manager, &manager);
#endif

  // Wire the manager to our models
  manager.setMessageModel(&msgModel);
  manager.setServerChannelModel(&treeModel);
  manager.setLogger(&logger);
  manager.setSettings(&appSettings);

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
  engine.rootContext()->setContextProperty("imgDownloader",
                                           ImageDownloader::instance());
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
          QCoreApplication::exit(-1);
        }
      },
      Qt::QueuedConnection);
  engine.load(url);
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
