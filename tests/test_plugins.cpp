#include <QtTest>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "IrcConnection.h"
#include "IrcLineParse.h"
#include "LuaAlloc.h"
#include "PluginManager.h"
#include "ScriptDispatch.h"
#include "ScriptEat.h"
#include "ScriptManager.h"

#ifdef HAVE_LUA
#include "LuaScriptEngine.h"
#endif
#ifdef HAVE_PYTHON
#include "PythonScriptEngine.h"
#endif

class IrcConnectionTestable : public IrcConnection
{
public:
    using IrcConnection::IrcConnection;
    void feedLine(const QString &line) { processLine(line); }
};

class ScriptManagerTestAccess
{
public:
#if HAVE_QJSE
    static QJSValue eval(ScriptManager &mgr, const QString &code)
    {
        return mgr.m_engine.evaluate(code);
    }
#endif
};

class TestPlugins : public QObject
{
    Q_OBJECT

private slots:
    void luaAllocAccountingFollowsContract()
    {
        LuaAllocState st;
        st.limit = 1024;
        // ptr == NULL: osize is a type tag (here 4), not a size
        void *p = luaLimitedAlloc(&st, nullptr, 4, 100);
        QVERIFY(p);
        QCOMPARE(st.used, size_t(100));
        // grow, shrink
        p = luaLimitedAlloc(&st, p, 100, 300);
        QVERIFY(p);
        QCOMPARE(st.used, size_t(300));
        p = luaLimitedAlloc(&st, p, 300, 50);
        QVERIFY(p);
        QCOMPARE(st.used, size_t(50));
        // over the limit: denied, old block and accounting untouched
        QVERIFY(!luaLimitedAlloc(&st, p, 50, 5000));
        QCOMPARE(st.used, size_t(50));
        QVERIFY(!luaLimitedAlloc(&st, nullptr, 3, 5000));
        QCOMPARE(st.used, size_t(50));
        // free returns to zero
        QVERIFY(!luaLimitedAlloc(&st, p, 50, 0));
        QCOMPARE(st.used, size_t(0));
        // freeing NULL is a no-op even with a tag in osize
        QVERIFY(!luaLimitedAlloc(&st, nullptr, 4, 0));
        QCOMPARE(st.used, size_t(0));
        // many tagged allocations and frees do not drift
        void *blocks[200];
        for (int i = 0; i < 200; ++i) {
            blocks[i] = luaLimitedAlloc(&st, nullptr, size_t(i % 9), 4);
            QVERIFY(blocks[i]);
        }
        QCOMPARE(st.used, size_t(800));
        for (int i = 0; i < 200; ++i)
            luaLimitedAlloc(&st, blocks[i], 4, 0);
        QCOMPARE(st.used, size_t(0));
    }

    void testParseCommandSkipsTagsAndPrefix();
    void testParseUnprefixedPing();
    void testParseNumeric();
    void testBangCommandFromPrivmsg();
    void testBangCommandIgnoresRawBangAndCtcp();
    void testFoldEat();
    void testEatSkipsCoreHandling();
    void testTaggedPrivmsgStillParsed();
    void testScriptDirectoriesFindHello();
    void testJsOnMessageGetsPrivmsgText();
    void testExamplePluginLoadsAndHandlesHello();
#ifdef HAVE_LUA
    void testLuaServerHooksMatchTagsAndPing();
    void testLuaPriorityAndEatPlugin();
    void testLuaUnhookDuringServerHook();
    void testLuaTimerRemovesItself();
#endif
#ifdef HAVE_PYTHON
    void testPythonServerHooksMatchTagsAndPing();
    void testPythonPriorityAndEatPlugin();
    void testPythonUnhookDuringServerHook();
    void testPythonTimerRemovesItself();
#endif
#if defined(HAVE_LUA) && defined(HAVE_PYTHON)
    void testCrossEngineEatPluginSkipsLua();
#endif
};

void TestPlugins::testParseCommandSkipsTagsAndPrefix()
{
    const auto view = parseIrcLine(
        "@time=2026-10-16T00:00:00.000Z;account=bob :bob!b@h PRIVMSG #chan :hello world");
    QCOMPARE(view.command, QString("PRIVMSG"));
    QCOMPARE(view.prefix, QString("bob!b@h"));
    QVERIFY(view.tags.contains("account=bob"));
    QCOMPARE(view.parts.value(0), QString(":bob!b@h"));
    QCOMPARE(view.parts.value(1), QString("PRIVMSG"));
    QCOMPARE(ircTrailing("@time=1 :bob!b@h PRIVMSG #chan :hello world"), QString("hello world"));
    QCOMPARE(ircNickFromPrefix(view.prefix), QString("bob"));
}

void TestPlugins::testParseUnprefixedPing()
{
    const auto view = parseIrcLine("PING :irc.example.net");
    QCOMPARE(view.command, QString("PING"));
    QVERIFY(view.prefix.isEmpty());
    QCOMPARE(view.parts.value(0), QString("PING"));
    QCOMPARE(view.parts.value(1), QString(":irc.example.net"));
}

void TestPlugins::testParseNumeric()
{
    const auto view = parseIrcLine("@time=1 :irc.example.net 001 mynick :Welcome");
    QCOMPARE(view.command, QString("001"));
    QCOMPARE(ircTrailing("@time=1 :irc.example.net 001 mynick :Welcome"), QString("Welcome"));
}

void TestPlugins::testBangCommandFromPrivmsg()
{
    const auto bang = bangCommandFromRaw(
        "@time=2026-10-16T00:00:00.000Z :bob!b@h PRIVMSG #chan :!Hello there friend");
    QCOMPARE(bang.command, QString("hello"));
    QCOMPARE(bang.args, QStringList() << "there" << "friend");

    const auto notice = bangCommandFromRaw(":bob!b@h NOTICE nick :!Ping");
    QCOMPARE(notice.command, QString("ping"));
    QVERIFY(notice.args.isEmpty());
}

void TestPlugins::testBangCommandIgnoresRawBangAndCtcp()
{
    QVERIFY(bangCommandFromRaw("!hello").command.isEmpty());
    QVERIFY(bangCommandFromRaw("PING :x").command.isEmpty());
    QVERIFY(bangCommandFromRaw(":bob!b@h PRIVMSG #c :hello").command.isEmpty());
    QVERIFY(bangCommandFromRaw(":bob!b@h PRIVMSG #c :\x01PING 123\x01").command.isEmpty());
    QVERIFY(bangCommandFromRaw(":bob!b@h PRIVMSG #c :!").command.isEmpty());
}

void TestPlugins::testFoldEat()
{
    QCOMPARE(foldEat(NUCHAT_EAT_NONE, NUCHAT_EAT_HEXCHAT), NUCHAT_EAT_HEXCHAT);
    QCOMPARE(foldEat(NUCHAT_EAT_HEXCHAT, NUCHAT_EAT_PLUGIN), NUCHAT_EAT_ALL);
    // foldEat only ORs bits. Callers skip later hooks themselves once
    // EAT_PLUGIN is set, so a second return is not normally folded in.
    QCOMPARE(foldEat(NUCHAT_EAT_PLUGIN, NUCHAT_EAT_ALL), NUCHAT_EAT_ALL);
    QVERIFY(eatSuppressesCore(NUCHAT_EAT_ALL));
    QVERIFY(eatStopsHooks(NUCHAT_EAT_PLUGIN));
    QVERIFY(!eatSuppressesCore(NUCHAT_EAT_PLUGIN));
    QVERIFY(!eatStopsHooks(NUCHAT_EAT_HEXCHAT));
}

void TestPlugins::testEatSkipsCoreHandling()
{
    IrcConnectionTestable conn;
    QSignalSpy spy(&conn, &IrcConnection::privmsgReceived);
    QObject::connect(&conn, &IrcConnection::rawLineReceived, &conn,
                     [&conn](const QString &) { conn.eatCurrentLine(); });
    conn.feedLine(":nick!u@h PRIVMSG #c :hello");
    QCOMPARE(spy.count(), 0);
}

void TestPlugins::testTaggedPrivmsgStillParsed()
{
    IrcConnectionTestable conn;
    QSignalSpy spy(&conn, &IrcConnection::privmsgReceived);
    conn.feedLine("@time=2026-10-16T00:00:00.000Z :nick!u@h PRIVMSG #c :hello");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(2).toString(), QString("hello"));
}

void TestPlugins::testScriptDirectoriesFindHello()
{
    bool found = false;
    const QStringList dirs = ScriptManager::candidateDirectories();
    for (const QString &dir : dirs) {
        if (QFile::exists(dir + QStringLiteral("/hello.js")))
            found = true;
    }
    QVERIFY2(found, "ScriptManager did not find the in-tree scripts directory");
}

void TestPlugins::testJsOnMessageGetsPrivmsgText()
{
#if !HAVE_QJSE
    QSKIP("QJSEngine is not available");
#else
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath("t.js"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("var seen = [];\n"
               "function onMessage(conn, sender, message) {\n"
               "  seen.push(sender + '|' + message);\n"
               "}\n");
    file.close();

    ScriptManager mgr;
    mgr.loadScripts(dir.path());
    IrcConnection conn;
    mgr.handleRawServerLine(&conn, "@time=1 :alice!a@b PRIVMSG #c :!ping");
    mgr.handleRawServerLine(&conn, "!ping");
    mgr.handleRawServerLine(&conn, "PING :irc.example.net");
    mgr.handleRawServerLine(&conn, ":bob!b@h NOTICE alice :hello");

    const QJSValue joined = ScriptManagerTestAccess::eval(mgr, "seen.join(',')");
    QCOMPARE(joined.toString(), QString("alice|!ping,bob|hello"));
#endif
}

void TestPlugins::testExamplePluginLoadsAndHandlesHello()
{
    PluginManager mgr;
    const QString dir = QStringLiteral(NUCHAT_PLUGIN_BUILD_DIR);
    mgr.loadPlugins(dir);
    if (mgr.loadedCount() != 1) {
        qWarning() << "plugins in" << dir
                   << QDir(dir).entryList(QDir::Files);
    }
    QCOMPARE(mgr.loadedCount(), 1);
    QVERIFY(mgr.handleUserCommand("hello", {}, nullptr));
    QVERIFY(mgr.handleUserCommand("HELLO", {}, nullptr));
    QVERIFY(!mgr.handleUserCommand("nope", {}, nullptr));

    IrcConnection conn;
    mgr.onMessage(&conn, "@time=1 :bob!b@h PRIVMSG #general :!hello there");
    mgr.onMessage(&conn, "!hello");
    mgr.onMessage(&conn, "PING :x");
}

#ifdef HAVE_LUA
static QString writeTempScript(QTemporaryDir &dir, const char *name, const char *body)
{
    const QString path = dir.filePath(QString::fromUtf8(name));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(body);
    file.close();
    return path;
}

void TestPlugins::testLuaServerHooksMatchTagsAndPing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeTempScript(dir, "srv.lua",
        "hexchat.hook_server('PRIVMSG', function(word, word_eol, userdata)\n"
        "  return hexchat.EAT_PLUGIN\n"
        "end)\n"
        "hexchat.hook_server('PING', function(word, word_eol, userdata)\n"
        "  return hexchat.EAT_ALL\n"
        "end)\n");
    QVERIFY(!path.isEmpty());

    LuaScriptEngine engine(nullptr);
    engine.loadScript(path);
    QCOMPARE(engine.hookCount(), 2);

    QCOMPARE(engine.handleServerLine(nullptr,
                 "@time=2026-10-16T00:00:00.000Z :n!u@h PRIVMSG #c :hi"),
             NUCHAT_EAT_PLUGIN);
    QCOMPARE(engine.handleServerLine(nullptr, "PING :irc.example.net"), NUCHAT_EAT_ALL);
    QCOMPARE(engine.handleServerLine(nullptr, ":n!u@h NOTICE #c :x"), NUCHAT_EAT_NONE);
    QCOMPARE(engine.handleCommand("hello", {}), NUCHAT_EAT_NONE);
}

void TestPlugins::testLuaPriorityAndEatPlugin()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeTempScript(dir, "pri.lua",
        "hexchat.hook_server('NOTICE', function() return hexchat.EAT_HEXCHAT end, nil, hexchat.PRI_LOW)\n"
        "hexchat.hook_server('NOTICE', function() return hexchat.EAT_PLUGIN end, nil, hexchat.PRI_HIGH)\n"
        "hexchat.hook_command('NOPE', function() return hexchat.EAT_HEXCHAT end, nil, hexchat.PRI_LOW)\n"
        "hexchat.hook_command('NOPE', function() return hexchat.EAT_PLUGIN end, nil, hexchat.PRI_HIGH)\n");
    QVERIFY(!path.isEmpty());

    LuaScriptEngine engine(nullptr);
    engine.loadScript(path);
    QCOMPARE(engine.handleServerLine(nullptr, ":n!u@h NOTICE #c :x"), NUCHAT_EAT_PLUGIN);
    QCOMPARE(engine.handleCommand("nope", QStringList()), NUCHAT_EAT_PLUGIN);
}

void TestPlugins::testLuaUnhookDuringServerHook()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeTempScript(dir, "unhook.lua",
        "local id\n"
        "id = hexchat.hook_server('JOIN', function()\n"
        "  hexchat.unhook(id)\n"
        "  hexchat.hook_server('JOIN', function() return hexchat.EAT_NONE end)\n"
        "  return hexchat.EAT_NONE\n"
        "end)\n"
        "hexchat.hook_server('JOIN', function() return hexchat.EAT_HEXCHAT end)\n");
    QVERIFY(!path.isEmpty());

    LuaScriptEngine engine(nullptr);
    engine.loadScript(path);
    QCOMPARE(engine.handleServerLine(nullptr, ":n!u@h JOIN #c"), NUCHAT_EAT_HEXCHAT);
}

void TestPlugins::testLuaTimerRemovesItself()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = writeTempScript(dir, "timer.lua",
        "local id\n"
        "id = hexchat.hook_timer(20, function()\n"
        "  hexchat.unhook(id)\n"
        "  return 0\n"
        "end)\n"
        "hexchat.hook_timer(20, function() return 0 end)\n");
    QVERIFY(!path.isEmpty());

    LuaScriptEngine engine(nullptr);
    engine.loadScript(path);
    QCOMPARE(engine.hookCount(), 2);
    QTest::qWait(150);
    QCOMPARE(engine.hookCount(), 0);
}
#endif

#ifdef HAVE_PYTHON
void TestPlugins::testPythonServerHooksMatchTagsAndPing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath("srv.py"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("import hexchat\n"
               "def priv(word, word_eol, userdata):\n"
               "    return hexchat.EAT_PLUGIN\n"
               "def ping(word, word_eol, userdata):\n"
               "    return hexchat.EAT_ALL\n"
               "hexchat.hook_server('PRIVMSG', priv)\n"
               "hexchat.hook_server('PING', ping)\n");
    file.close();

    PythonScriptEngine engine(nullptr);
    engine.loadScript(file.fileName());
    if (engine.hookCount() == 0)
        QSKIP("Python interpreter did not load the test script");

    QCOMPARE(engine.handleServerLine(nullptr,
                 "@time=2026-10-16T00:00:00.000Z :n!u@h PRIVMSG #c :hi"),
             NUCHAT_EAT_PLUGIN);
    QCOMPARE(engine.handleServerLine(nullptr, "PING :irc.example.net"), NUCHAT_EAT_ALL);
    QCOMPARE(engine.handleServerLine(nullptr, ":n!u@h NOTICE #c :x"), NUCHAT_EAT_NONE);
}

void TestPlugins::testPythonPriorityAndEatPlugin()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath("pri.py"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("import hexchat\n"
               "def low(word, word_eol, userdata):\n"
               "    return hexchat.EAT_HEXCHAT\n"
               "def high(word, word_eol, userdata):\n"
               "    return hexchat.EAT_PLUGIN\n"
               "hexchat.hook_server('NOTICE', low, None, hexchat.PRI_LOW)\n"
               "hexchat.hook_server('NOTICE', high, None, hexchat.PRI_HIGH)\n");
    file.close();

    PythonScriptEngine engine(nullptr);
    engine.loadScript(file.fileName());
    if (engine.hookCount() == 0)
        QSKIP("Python interpreter did not load the test script");
    QCOMPARE(engine.handleServerLine(nullptr, ":n!u@h NOTICE #c :x"), NUCHAT_EAT_PLUGIN);
}

void TestPlugins::testPythonUnhookDuringServerHook()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath("unhook.py"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("import hexchat\n"
               "ids = {}\n"
               "def first(word, word_eol, userdata):\n"
               "    hexchat.unhook(ids['a'])\n"
               "    def extra(word, word_eol, userdata):\n"
               "        return hexchat.EAT_NONE\n"
               "    hexchat.hook_server('PRIVMSG', extra)\n"
               "    return hexchat.EAT_NONE\n"
               "ids['a'] = hexchat.hook_server('PRIVMSG', first)\n"
               "def second(word, word_eol, userdata):\n"
               "    return hexchat.EAT_HEXCHAT\n"
               "hexchat.hook_server('PRIVMSG', second)\n");
    file.close();

    PythonScriptEngine engine(nullptr);
    engine.loadScript(file.fileName());
    if (engine.hookCount() < 2)
        QSKIP("Python interpreter did not load the test script");
    QCOMPARE(engine.handleServerLine(nullptr, ":n!u@h PRIVMSG #c :hi"), NUCHAT_EAT_HEXCHAT);
}

void TestPlugins::testPythonTimerRemovesItself()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath("timer.py"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("import hexchat\n"
               "ids = {}\n"
               "def once(userdata):\n"
               "    hexchat.unhook(ids['id'])\n"
               "    return 0\n"
               "ids['id'] = hexchat.hook_timer(20, once)\n"
               "def once2(userdata):\n"
               "    return 0\n"
               "hexchat.hook_timer(20, once2)\n");
    file.close();

    PythonScriptEngine engine(nullptr);
    engine.loadScript(file.fileName());
    if (engine.hookCount() != 2)
        QSKIP("Python interpreter did not load the test script");
    QTest::qWait(200);
    QCOMPARE(engine.hookCount(), 0);
}
#endif

#if defined(HAVE_LUA) && defined(HAVE_PYTHON)
void TestPlugins::testCrossEngineEatPluginSkipsLua()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile py(dir.filePath("cross.py"));
    QVERIFY(py.open(QIODevice::WriteOnly));
    py.write("import hexchat\n"
             "def mode(word, word_eol, userdata):\n"
             "    return hexchat.EAT_PLUGIN\n"
             "def kick(word, word_eol, userdata):\n"
             "    return hexchat.EAT_HEXCHAT\n"
             "hexchat.hook_server('MODE', mode)\n"
             "hexchat.hook_server('KICK', kick)\n");
    py.close();
    QFile lua(dir.filePath("cross.lua"));
    QVERIFY(lua.open(QIODevice::WriteOnly));
    lua.write("hexchat.hook_server('MODE', function() return hexchat.EAT_ALL end)\n"
              "hexchat.hook_server('KICK', function() return hexchat.EAT_PLUGIN end)\n");
    lua.close();

    PythonScriptEngine pyEngine(nullptr);
    LuaScriptEngine luaEngine(nullptr);
    pyEngine.loadScript(py.fileName());
    luaEngine.loadScript(lua.fileName());
    if (pyEngine.hookCount() < 2)
        QSKIP("Python interpreter did not load the test script");

    // Python EAT_PLUGIN must stop before Lua can OR in EAT_ALL.
    QCOMPARE(dispatchScriptServerLine(nullptr, ":n!u@h MODE #c +o x"), NUCHAT_EAT_PLUGIN);
    // Python EAT_HEXCHAT does not stop Lua, so Lua's EAT_PLUGIN is included.
    QCOMPARE(dispatchScriptServerLine(nullptr, ":n!u@h KICK #c x :bye"), NUCHAT_EAT_ALL);
}
#endif

QTEST_GUILESS_MAIN(TestPlugins)
#include "test_plugins.moc"
