#include <QtTest>
#include <QTemporaryDir>
#include "MessageModel.h"
#include "Settings.h"

class TestHexChatImport : public QObject
{
    Q_OBJECT

private slots:
    // Full network entry: identity, SSL port, autojoin, perform
    void testParseServlist()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QString path = dir.filePath("servlist.conf");
        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
            QTextStream out(&f);
            out << "v=2.16.2\n"
                << "\n"
                << "N=TestNet\n"
                << "E=UTF-8 (Unicode)\n"
                << "F=23\n"
                << "S=irc.test.net/+6697\n"
                << "S=backup.test.net/6667\n"
                << "I=mynick\n"
                << "U=myuser\n"
                << "R=My Real Name\n"
                << "P=secretpass\n"
                << "J=#chan1\n"
                << "J=#chan2\n"
                << "C=/msg NickServ IDENTIFY hunter2\n"
                << "\n"
                << "N=PlainNet\n"
                << "S=irc.plain.org\n";
        }

        QVariantList nets = Settings::parseHexChatServlist(path);
        QCOMPARE(nets.size(), 2);

        QVariantMap n1 = nets[0].toMap();
        QCOMPARE(n1["network"].toString(), QString("TestNet"));
        QCOMPARE(n1["server"].toString(), QString("irc.test.net"));
        QCOMPARE(n1["port"].toInt(), 6697);
        QCOMPARE(n1["ssl"].toBool(), true);
        // HexChat: I= is the per-network NICK, U= the username
        QCOMPARE(n1["customNick"].toString(), QString("mynick"));
        QCOMPARE(n1["customUser"].toString(), QString("myuser"));
        QCOMPARE(n1["customReal"].toString(), QString("My Real Name"));
        QCOMPARE(n1["useGlobalNick"].toBool(), false);
        QCOMPARE(n1["serverPass"].toString(), QString("secretpass"));
        QCOMPARE(n1["autojoin"].toString(), QString("#chan1,#chan2"));
        QCOMPARE(n1["perform"].toString(), QString("/msg NickServ IDENTIFY hunter2"));

        QVariantMap n2 = nets[1].toMap();
        QCOMPARE(n2["network"].toString(), QString("PlainNet"));
        QCOMPARE(n2["server"].toString(), QString("irc.plain.org"));
        QCOMPARE(n2["port"].toInt(), 6667);
        QCOMPARE(n2["ssl"].toBool(), false);
        QCOMPARE(n2["useGlobalNick"].toBool(), true);
        QCOMPARE(n2["autojoin"].toString(), QString());
        QCOMPARE(n2["perform"].toString(), QString());
    }

    void testParseServlistMissingFile()
    {
        QVariantList nets =
            Settings::parseHexChatServlist("/nonexistent/servlist.conf");
        QVERIFY(nets.isEmpty());
    }

    // Whole-word nick matching used for highlight detection
    void testContainsNickWord()
    {
        // Plain word matches
        QVERIFY(MessageModel::containsNickWord("hey ed, how are you", "ed"));
        QVERIFY(MessageModel::containsNickWord("ed: ping", "ed"));
        QVERIFY(MessageModel::containsNickWord("thanks ed", "ed"));
        QVERIFY(MessageModel::containsNickWord("ED is here", "ed")); // case-insensitive

        // Substrings must NOT match
        QVERIFY(!MessageModel::containsNickWord("I edited the file", "ed"));
        QVERIFY(!MessageModel::containsNickWord("fred said hi", "ed"));
        QVERIFY(!MessageModel::containsNickWord("education", "ed"));

        // Nicks with special IRC characters
        QVERIFY(MessageModel::containsNickWord("hi [bob], sup", "[bob]"));
        QVERIFY(!MessageModel::containsNickWord("x[bob]y", "[bob]"));
        QVERIFY(MessageModel::containsNickWord("ping nick|away please", "nick|away"));

        // Empty nick never matches
        QVERIFY(!MessageModel::containsNickWord("anything", ""));
    }
};

QTEST_MAIN(TestHexChatImport)
#include "test_hexchatimport.moc"
