// Tests for IRC line parsing via IrcConnection signals.
// We use a thin test-helper subclass to expose the private processLine()
// method so we can feed raw server lines without a real TCP socket.
#include <QtTest>
#include "IrcConnection.h"

// Friend trick: expose processLine() for testing
class IrcConnectionTestable : public IrcConnection
{
public:
    using IrcConnection::IrcConnection;
    void feedLine(const QString &line) { processLine(line); }
};

class TestIrcParsing : public QObject
{
    Q_OBJECT

private slots:
    // PRIVMSG parsing
    void testPrivmsg()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::privmsgReceived);
        conn.feedLine(":nick!user@host PRIVMSG #channel :hello world");
        QCOMPARE(spy.count(), 1);
        auto args = spy.takeFirst();
        QCOMPARE(args[0].toString(), QString("nick!user@host"));
        QCOMPARE(args[1].toString(), QString("#channel"));
        QCOMPARE(args[2].toString(), QString("hello world"));
    }

    void testPrivmsgNoColon()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::privmsgReceived);
        conn.feedLine(":nick!user@host PRIVMSG #channel :single");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first()[2].toString(), QString("single"));
    }

    // NOTICE parsing
    void testNotice()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::noticeReceived);
        conn.feedLine(":server NOTICE nick :Connecting...");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first()[2].toString(), QString("Connecting..."));
    }

    // JOIN parsing
    void testJoin()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::joinReceived);
        conn.feedLine(":alice!a@b JOIN #channel");
        QCOMPARE(spy.count(), 1);
        auto args = spy.takeFirst();
        QCOMPARE(args[0].toString(), QString("alice!a@b"));
        QCOMPARE(args[1].toString(), QString("#channel"));
    }

    // PART parsing
    void testPart()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::partReceived);
        conn.feedLine(":alice!a@b PART #channel :Goodbye");
        QCOMPARE(spy.count(), 1);
        auto args = spy.takeFirst();
        QCOMPARE(args[0].toString(), QString("alice!a@b"));
        QCOMPARE(args[1].toString(), QString("#channel"));
        QCOMPARE(args[2].toString(), QString("Goodbye"));
    }

    // QUIT parsing
    void testQuit()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::quitReceived);
        conn.feedLine(":alice!a@b QUIT :Quit: bye");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first()[1].toString(), QString("Quit: bye"));
    }

    // NICK change parsing
    void testNickChange()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::nickChanged);
        conn.feedLine(":alice!a@b NICK :bob");
        QCOMPARE(spy.count(), 1);
        auto args = spy.takeFirst();
        QCOMPARE(args[0].toString(), QString("alice"));
        QCOMPARE(args[1].toString(), QString("bob"));
    }

    // KICK parsing
    void testKick()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::kickReceived);
        conn.feedLine(":op!o@h KICK #channel alice :spamming");
        QCOMPARE(spy.count(), 1);
        auto args = spy.takeFirst();
        QCOMPARE(args[0].toString(), QString("op!o@h"));
        QCOMPARE(args[1].toString(), QString("#channel"));
        QCOMPARE(args[2].toString(), QString("alice"));
        QCOMPARE(args[3].toString(), QString("spamming"));
    }

    // TOPIC parsing
    void testTopic()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::topicReceived);
        conn.feedLine(":alice!a@b TOPIC #channel :New topic here");
        QCOMPARE(spy.count(), 1);
        auto args = spy.takeFirst();
        QCOMPARE(args[0].toString(), QString("#channel"));
        QCOMPARE(args[1].toString(), QString("New topic here"));
    }

    // PING/PONG: no public signal — just verify no crash
    void testPingNoCrash()
    {
        IrcConnectionTestable conn;
        conn.feedLine("PING :server.example.com");
    }

    // Numeric RPL_WELCOME (001) — triggers nicknameChanged
    void testRplWelcome()
    {
        IrcConnectionTestable conn;
        conn.setNickname("testnick");
        QSignalSpy spy(&conn, &IrcConnection::registered);
        conn.feedLine(":irc.example.com 001 testnick :Welcome to the IRC network testnick!u@h");
        QCOMPARE(spy.count(), 1);
    }

    // Numeric RPL_NAMREPLY (353) — triggers namesReceived
    void testNamesReply()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::namesReceived);
        conn.feedLine(":server 353 me = #channel :@alice +bob charlie");
        // 353 alone doesn't emit — 366 triggers it
        conn.feedLine(":server 366 me #channel :End of /NAMES list");
        QCOMPARE(spy.count(), 1);
        QStringList names = spy.first()[1].toStringList();
        QVERIFY(!names.isEmpty());
    }

    // CTCP ACTION
    void testCtcpAction()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::privmsgReceived);
        QSignalSpy ctcpSpy(&conn, &IrcConnection::ctcpReceived);
        // CTCP ACTION is a PRIVMSG with \x01ACTION ...\x01
        conn.feedLine(":alice!a@b PRIVMSG #chan :\x01" "ACTION waves\x01");
        // Either emitted as ctcp or as privmsg depending on implementation
        QVERIFY(spy.count() + ctcpSpy.count() >= 1);
    }

    // Lines without a prefix (server commands)
    void testNoPrefixLine()
    {
        IrcConnectionTestable conn;
        // ERROR line has no prefix
        conn.feedLine("ERROR :Closing link");
    }

    // IRCv3 message-tag value unescaping
    void testTagUnescaping()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::taggedMessageReceived);
        conn.feedLine("@time=2026-01-01T00:00:00Z;plain=hello "
                      ":alice!a@b PRIVMSG #chan :hi");
        QCOMPARE(spy.count(), 1);
        auto tags = spy.takeFirst()[0].value<QMap<QString, QString>>();
        QCOMPARE(tags.value("time"), QString("2026-01-01T00:00:00Z"));
        QCOMPARE(tags.value("plain"), QString("hello"));
    }

    void testTagUnescapingEscapes()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::taggedMessageReceived);
        // \s = space, \: = semicolon, \\ = backslash, \\s must decode to
        // literal "\s" (backslash + s), NOT backslash-then-reinterpreted-space
        conn.feedLine("@a=one\\stwo;b=x\\:y;c=back\\\\slash;d=lit\\\\s;"
                      "e=bad\\qesc;f=trail\\ "
                      ":alice!a@b PRIVMSG #chan :hi");
        QCOMPARE(spy.count(), 1);
        auto tags = spy.takeFirst()[0].value<QMap<QString, QString>>();
        QCOMPARE(tags.value("a"), QString("one two"));
        QCOMPARE(tags.value("b"), QString("x;y"));
        QCOMPARE(tags.value("c"), QString("back\\slash"));
        QCOMPARE(tags.value("d"), QString("lit\\s"));
        // Invalid escape: backslash dropped, character kept
        QCOMPARE(tags.value("e"), QString("badqesc"));
        // Lone trailing backslash: no output character
        QCOMPARE(tags.value("f"), QString("trail"));
    }

    // Boolean tags (no value) parse as present-but-empty
    void testBooleanTag()
    {
        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::taggedMessageReceived);
        conn.feedLine("@solanum.chat/oper;account=alice "
                      ":alice!a@b PRIVMSG #chan :hi");
        QCOMPARE(spy.count(), 1);
        auto tags = spy.takeFirst()[0].value<QMap<QString, QString>>();
        QVERIFY(tags.contains("solanum.chat/oper"));
        QCOMPARE(tags.value("account"), QString("alice"));
    }
};

QTEST_MAIN(TestIrcParsing)
#include "test_ircparsing.moc"
