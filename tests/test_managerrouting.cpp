#include <QtTest>
#include "IRCConnectionManager.h"
#include "IrcConnection.h"
#include "MessageModel.h"
#include "ServerChannelModel.h"
#include "Settings.h"

class ManagerRoutingTest : public QObject
{
    Q_OBJECT

    static bool historyHas(IRCConnectionManager &mgr, const QString &server,
                           const QString &channel, const QString &needle)
    {
        const IRCConnectionManager::ChannelKey key{server, channel};
        const auto it = mgr.m_history.constFind(key);
        if (it == mgr.m_history.constEnd())
            return false;
        for (const auto &msg : *it) {
            if (msg.text.contains(needle))
                return true;
        }
        return false;
    }

    static QString modelText(const MessageModel &model)
    {
        QString out;
        for (int i = 0; i < model.rowCount(); ++i)
            out += model.data(model.index(i), MessageModel::TextRole).toString();
        return out;
    }

    static IrcConnection *attach(IRCConnectionManager &mgr, ServerChannelModel &tree,
                                 const QString &name)
    {
        auto *conn = new IrcConnection;
        conn->setNickname(QStringLiteral("Alice"));
        mgr.m_connections.append(conn);
        mgr.m_connToName.insert(conn, name);
        mgr.wireConnection(conn);
        tree.addServer(name);
        return conn;
    }

    static void emitNumeric(IrcConnection *conn, int code, const QStringList &params,
                            const QString &trailing)
    {
        conn->numericReceived(code, params, trailing);
    }

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName("NUchatTestRoute");
        QCoreApplication::setApplicationName("NUchatTestRoute");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_tempDir->path());
    }

    void testWhowasStaysOnOriginServer()
    {
        IRCConnectionManager mgr;
        MessageModel model;
        ServerChannelModel tree;
        mgr.setMessageModel(&model);
        mgr.setServerChannelModel(&tree);
        IrcConnection *alpha = attach(mgr, tree, QStringLiteral("alpha"));
        attach(mgr, tree, QStringLiteral("beta"));

        mgr.m_activeServer = QStringLiteral("beta");
        mgr.m_activeChannel = QStringLiteral("#other");
        model.setBufferContext(QStringLiteral("beta"), QStringLiteral("#other"));

        emitNumeric(alpha, 314,
                    QStringList() << QStringLiteral("x") << QStringLiteral("oldnick")
                                  << QStringLiteral("u") << QStringLiteral("h"),
                    QStringLiteral("real"));

        QVERIFY(!modelText(model).contains(QStringLiteral("WHOWAS")));
        QVERIFY(!historyHas(mgr, QStringLiteral("beta"), QStringLiteral("#other"),
                            QStringLiteral("WHOWAS")));
        QVERIFY(historyHas(mgr, QStringLiteral("alpha"), QStringLiteral("alpha"),
                           QStringLiteral("[WHOWAS] oldnick was u@h : real")));

        mgr.switchToChannel(QStringLiteral("alpha"), QStringLiteral("#here"));
        emitNumeric(alpha, 314,
                    QStringList() << QStringLiteral("x") << QStringLiteral("othernick")
                                  << QStringLiteral("u") << QStringLiteral("h"),
                    QStringLiteral("name"));
        QVERIFY(modelText(model).contains(QStringLiteral("othernick")));
        QVERIFY(historyHas(mgr, QStringLiteral("alpha"), QStringLiteral("#here"),
                           QStringLiteral("othernick")));
        QVERIFY(!historyHas(mgr, QStringLiteral("beta"), QStringLiteral("#other"),
                            QStringLiteral("othernick")));
    }

    void testAwayStateIsPerServer()
    {
        IRCConnectionManager mgr;
        MessageModel model;
        ServerChannelModel tree;
        mgr.setMessageModel(&model);
        mgr.setServerChannelModel(&tree);
        IrcConnection *alpha = attach(mgr, tree, QStringLiteral("alpha"));
        IrcConnection *beta = attach(mgr, tree, QStringLiteral("beta"));

        mgr.m_activeServer = QStringLiteral("alpha");
        mgr.m_activeChannel = QStringLiteral("#chan");
        QVERIFY(mgr.handleSlashCommand(alpha, QStringLiteral("#chan"),
                                        QStringLiteral("AWAY"),
                                        QStringLiteral("lunch")));
        QVERIFY(mgr.isAway());
        QVERIFY(mgr.m_awayByServer.value(QStringLiteral("alpha")).away);
        QVERIFY(!mgr.m_awayByServer.value(QStringLiteral("beta")).away);

        mgr.appendToChannel(QStringLiteral("alpha"), QStringLiteral("#chan"),
                            QStringLiteral("chat"),
                            QStringLiteral("<bob> while away"));
        QCOMPARE(mgr.awayLog().size(), 1);
        QCOMPARE(mgr.awayLog().at(0).toMap().value(QStringLiteral("message")).toString(),
                 QStringLiteral("<bob> while away"));

        emitNumeric(beta, 306, QStringList(), QStringLiteral("You are away"));
        QVERIFY(mgr.isAway());
        QVERIFY(mgr.m_awayByServer.value(QStringLiteral("beta")).away);
        QCOMPARE(mgr.m_awayByServer.value(QStringLiteral("beta")).log.size(), 0);
        QVERIFY(!modelText(model).contains(QStringLiteral("You are away")));
        QVERIFY(historyHas(mgr, QStringLiteral("beta"), QStringLiteral("beta"),
                           QStringLiteral("You are away")));

        mgr.m_activeServer = QStringLiteral("beta");
        mgr.m_activeChannel = QStringLiteral("beta");
        QVERIFY(mgr.isAway());
        QCOMPARE(mgr.awayLog().size(), 0);
        mgr.appendToChannel(QStringLiteral("alpha"), QStringLiteral("#chan"),
                            QStringLiteral("chat"),
                            QStringLiteral("<bob> still on alpha"));
        QCOMPARE(mgr.awayLog().size(), 0);
        QCOMPARE(mgr.m_awayByServer.value(QStringLiteral("alpha")).log.size(), 2);

        const int rowsBefore = model.rowCount();
        emitNumeric(alpha, 305, QStringList(), QString());
        QCOMPARE(model.rowCount(), rowsBefore);
        QVERIFY(!mgr.m_awayByServer.value(QStringLiteral("alpha")).away);
        QVERIFY(mgr.m_awayByServer.value(QStringLiteral("beta")).away);
        QVERIFY(historyHas(mgr, QStringLiteral("alpha"), QStringLiteral("alpha"),
                           QStringLiteral("You are no longer marked as being away")));
        QCOMPARE(mgr.m_awayByServer.value(QStringLiteral("alpha")).log.size(), 2);

        mgr.m_activeServer = QStringLiteral("alpha");
        QVERIFY(!mgr.isAway());
        QCOMPARE(mgr.awayLog().size(), 2);
        QVERIFY(mgr.handleSlashCommand(alpha, QStringLiteral("#chan"),
                                        QStringLiteral("BACK"), QString()));
        QVERIFY(!mgr.isAway());
        QVERIFY(mgr.m_awayByServer.value(QStringLiteral("beta")).away);
    }

    void testUnreadAndTreeAreCaseInsensitive()
    {
        IRCConnectionManager mgr;
        MessageModel model;
        ServerChannelModel tree;
        mgr.setMessageModel(&model);
        mgr.setServerChannelModel(&tree);
        attach(mgr, tree, QStringLiteral("Alpha"));

        mgr.m_activeServer = QStringLiteral("beta");
        mgr.m_activeChannel = QStringLiteral("#other");
        mgr.appendToChannel(QStringLiteral("Alpha"), QStringLiteral("#Foo"),
                            QStringLiteral("chat"), QStringLiteral("<bob> hi"));
        QVERIFY(mgr.hasUnread(QStringLiteral("alpha"), QStringLiteral("#foo")));
        QVERIFY(historyHas(mgr, QStringLiteral("alpha"), QStringLiteral("#FOO"),
                           QStringLiteral("<bob> hi")));

        tree.addChannel(QStringLiteral("Alpha"), QStringLiteral("#Foo"));
        tree.addChannel(QStringLiteral("alpha"), QStringLiteral("#foo"));
        QVERIFY(tree.hasChannel(QStringLiteral("ALPHA"), QStringLiteral("#FOO")));
        QCOMPARE(tree.item(0)->rowCount(), 1);
        tree.removeChannel(QStringLiteral("alpha"), QStringLiteral("#foo"));
        QVERIFY(!tree.hasChannel(QStringLiteral("Alpha"), QStringLiteral("#Foo")));
    }

    void testSelfJoinMatchesNickCase()
    {
        IRCConnectionManager mgr;
        MessageModel model;
        ServerChannelModel tree;
        Settings settings;
        settings.setBool(QStringLiteral("ui/tabsInBackground"), false);
        mgr.setMessageModel(&model);
        mgr.setServerChannelModel(&tree);
        mgr.setSettings(&settings);
        IrcConnection *conn = attach(mgr, tree, QStringLiteral("alpha"));
        conn->setNickname(QStringLiteral("Alice"));

        QSignalSpy joined(&mgr, &IRCConnectionManager::channelJoined);
        conn->joinReceived(QStringLiteral("ALICE!a@b"), QStringLiteral("#foo"));
        QCOMPARE(joined.count(), 1);
        QVERIFY(tree.hasChannel(QStringLiteral("alpha"), QStringLiteral("#FOO")));
        QCOMPARE(mgr.m_activeChannel, QStringLiteral("#foo"));

        conn->joinReceived(QStringLiteral("alice!a@b"), QStringLiteral("#Foo"));
        QCOMPARE(tree.item(0)->rowCount(), 1);
        QCOMPARE(joined.count(), 2);
    }

    void testTopicSignalIsEscapedHtml()
    {
        IRCConnectionManager mgr;
        MessageModel model;
        ServerChannelModel tree;
        Settings settings;
        mgr.setMessageModel(&model);
        mgr.setServerChannelModel(&tree);
        mgr.setSettings(&settings);
        IrcConnection *conn = attach(mgr, tree, QStringLiteral("alpha"));
        mgr.m_activeServer = QStringLiteral("alpha");
        mgr.m_activeChannel = QStringLiteral("#foo");

        QSignalSpy topicSpy(&mgr, &IRCConnectionManager::channelTopicChanged);
        conn->topicReceived(
            QStringLiteral("#foo"),
            QStringLiteral("<img src=x onerror=alert(1)> <a href=\"file:///etc\">x</a>"));
        QCOMPARE(topicSpy.count(), 1);
        const QString html = topicSpy.first().at(0).toString();
        QVERIFY(!html.contains(QStringLiteral("<img")));
        QVERIFY(!html.contains(QStringLiteral("<a href=\"file")));
        QVERIFY(html.contains(QStringLiteral("&lt;img")));
        // Same output as the buffer-switch path
        QCOMPARE(html, mgr.channelTopic());
    }

    void testExternalUrlAllowlist()
    {
        QVERIFY(IRCConnectionManager::isSafeExternalUrl(QStringLiteral("https://example.com/a?b=1")));
        QVERIFY(IRCConnectionManager::isSafeExternalUrl(QStringLiteral("HTTP://example.com")));
        QVERIFY(IRCConnectionManager::isSafeExternalUrl(QStringLiteral("ircs://irc.example.net:6697/#chan")));
        QVERIFY(IRCConnectionManager::isSafeExternalUrl(QStringLiteral("mailto:a@b.c")));
        QVERIFY(!IRCConnectionManager::isSafeExternalUrl(QStringLiteral("file:///etc/passwd")));
        QVERIFY(!IRCConnectionManager::isSafeExternalUrl(QStringLiteral("javascript:alert(1)")));
        QVERIFY(!IRCConnectionManager::isSafeExternalUrl(QStringLiteral("smb://host/share")));
        QVERIFY(!IRCConnectionManager::isSafeExternalUrl(QStringLiteral("example.com")));
        QVERIFY(!IRCConnectionManager::isSafeExternalUrl(QString()));
    }

    void testTabsInBackgroundDoesNotStealFocus()
    {
        IRCConnectionManager mgr;
        ServerChannelModel tree;
        Settings settings;
        settings.setBool(QStringLiteral("ui/tabsInBackground"), true);
        mgr.setServerChannelModel(&tree);
        mgr.setSettings(&settings);
        IrcConnection *conn = attach(mgr, tree, QStringLiteral("alpha"));
        mgr.m_activeServer = QStringLiteral("alpha");
        mgr.m_activeChannel = QStringLiteral("alpha");

        conn->joinReceived(QStringLiteral("Alice!a@b"), QStringLiteral("#foo"));
        QCOMPARE(mgr.m_activeServer, QStringLiteral("alpha"));
        QCOMPARE(mgr.m_activeChannel, QStringLiteral("alpha"));
        QVERIFY(tree.hasChannel(QStringLiteral("alpha"), QStringLiteral("#foo")));
    }

    void testUnmappedConnectionDoesNotLogUnknown()
    {
        IRCConnectionManager mgr;
        MessageModel model;
        ServerChannelModel tree;
        mgr.setMessageModel(&model);
        mgr.setServerChannelModel(&tree);
        IrcConnection *conn = attach(mgr, tree, QStringLiteral("alpha"));
        mgr.m_connToName.remove(conn);

        emitNumeric(conn, 314,
                    QStringList() << QStringLiteral("x") << QStringLiteral("ghost")
                                  << QStringLiteral("u") << QStringLiteral("h"),
                    QStringLiteral("real"));
        QVERIFY(!modelText(model).contains(QStringLiteral("WHOWAS")));
        QVERIFY(!modelText(model).contains(QStringLiteral("unknown")));
        for (auto it = mgr.m_history.cbegin(); it != mgr.m_history.cend(); ++it) {
            QVERIFY(it.key().server.compare(QStringLiteral("unknown"),
                                             Qt::CaseInsensitive) != 0);
            for (const auto &msg : it.value())
                QVERIFY(!msg.text.contains(QStringLiteral("ghost")));
        }
    }

    void testUrlGrabberLegacyKey()
    {
        IRCConnectionManager mgr;
        Settings settings;
        settings.remove(QStringLiteral("url/autoGrab"));
        settings.setBool(QStringLiteral("url/grabber"), false);
        mgr.setSettings(&settings);
        mgr.appendToChannel(QStringLiteral("alpha"), QStringLiteral("#pub"),
                            QStringLiteral("chat"),
                            QStringLiteral("<bob> https://example.com/a"));
        QCOMPARE(mgr.grabbedUrls().size(), 0);

        settings.setBool(QStringLiteral("url/autoGrab"), true);
        mgr.appendToChannel(QStringLiteral("alpha"), QStringLiteral("#pub"),
                            QStringLiteral("chat"),
                            QStringLiteral("<bob> https://example.com/b"));
        QCOMPARE(mgr.grabbedUrls().size(), 1);
        QCOMPARE(mgr.grabbedUrls().at(0).toMap().value(QStringLiteral("url")).toString(),
                 QStringLiteral("https://example.com/b"));
    }

    void testEmbedIsReplayedOnChannelSwitch()
    {
        IRCConnectionManager mgr;
        MessageModel model;
        mgr.setMessageModel(&model);
        model.embedReady(QStringLiteral("alpha"), QStringLiteral("#one"),
                         QStringLiteral("<img src=\"file:///tmp/pic.png\">"));
        QVERIFY(historyHas(mgr, QStringLiteral("alpha"), QStringLiteral("#one"),
                           QStringLiteral("file:///tmp/pic.png")));

        mgr.m_activeServer = QStringLiteral("beta");
        mgr.m_activeChannel = QStringLiteral("#other");
        mgr.switchToChannel(QStringLiteral("alpha"), QStringLiteral("#one"));
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), MessageModel::TypeRole).toString(),
                 QStringLiteral("embed"));
        QVERIFY(modelText(model).contains(QStringLiteral("file:///tmp/pic.png")));
    }

private:
    QScopedPointer<QTemporaryDir> m_tempDir;
};

QTEST_MAIN(ManagerRoutingTest)
#include "test_managerrouting.moc"
