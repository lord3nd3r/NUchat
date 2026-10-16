#include <QtTest>
#include <QDebug>
#include <QSignalSpy>
#include "CredentialRedactor.h"
#include "IrcConnection.h"

class IrcConnectionTestable : public IrcConnection
{
public:
    using IrcConnection::IrcConnection;
    void feedLine(const QString &line) { processLine(line); }
};

namespace {
QString *g_captured = nullptr;
void captureMessage(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    if (g_captured)
        *g_captured += msg;
}
}

class TestRedact : public QObject
{
    Q_OBJECT

private slots:
    void testPassAndSasl()
    {
        using CredentialRedactor::redactIrcLine;
        QCOMPARE(redactIrcLine(QStringLiteral("PASS :hunter2")), QStringLiteral("PASS ***"));
        QCOMPARE(redactIrcLine(QStringLiteral("PASS hunter2")), QStringLiteral("PASS ***"));
        QCOMPARE(redactIrcLine(QStringLiteral("@x=y PASS :hunter2")),
                 QStringLiteral("@x=y PASS ***"));
        QCOMPARE(redactIrcLine(QStringLiteral("AUTHENTICATE PLAIN")),
                 QStringLiteral("AUTHENTICATE PLAIN"));
        QCOMPARE(redactIrcLine(QStringLiteral("AUTHENTICATE +")),
                 QStringLiteral("AUTHENTICATE +"));
        QCOMPARE(redactIrcLine(QStringLiteral("AUTHENTICATE SCRAM-SHA-256")),
                 QStringLiteral("AUTHENTICATE SCRAM-SHA-256"));

        QByteArray raw;
        raw.append("user");
        raw.append('\0');
        raw.append("user");
        raw.append('\0');
        raw.append("s3cret-pw");
        const QString b64 = QString::fromLatin1(raw.toBase64());
        const QString redacted = redactIrcLine(QStringLiteral("AUTHENTICATE ") + b64);
        QCOMPARE(redacted, QStringLiteral("AUTHENTICATE ***"));
        QVERIFY(!redacted.contains(b64));
        QVERIFY(!redacted.contains(QStringLiteral("s3cret-pw")));
    }

    void testOperAndServiceLines()
    {
        using CredentialRedactor::redactIrcLine;
        QCOMPARE(redactIrcLine(QStringLiteral("OPER admin hunter2")),
                 QStringLiteral("OPER admin ***"));
        QCOMPARE(redactIrcLine(QStringLiteral("OPER admin :hunter2 with space")),
                 QStringLiteral("OPER admin :***"));
        QCOMPARE(redactIrcLine(QStringLiteral(":n!u@h PRIVMSG NickServ :IDENTIFY hunter2")),
                 QStringLiteral(":n!u@h PRIVMSG NickServ :IDENTIFY ***"));
        QCOMPARE(redactIrcLine(QStringLiteral(":n!u@h PRIVMSG nickserv :IDENTIFY acc hunter2")),
                 QStringLiteral(":n!u@h PRIVMSG nickserv :IDENTIFY ***"));
        QCOMPARE(redactIrcLine(QStringLiteral("NOTICE NickServ :REGISTER hunter2 a@b.c")),
                 QStringLiteral("NOTICE NickServ :REGISTER ***"));
        // Channel chatter is not a services command, even if it says IDENTIFY.
        QCOMPARE(redactIrcLine(QStringLiteral(":n!u@h PRIVMSG #chan :IDENTIFY hunter2")),
                 QStringLiteral(":n!u@h PRIVMSG #chan :IDENTIFY hunter2"));
        QCOMPARE(redactIrcLine(QStringLiteral("PRIVMSG #c :hello")),
                 QStringLiteral("PRIVMSG #c :hello"));
        QCOMPARE(redactIrcLine(QStringLiteral("PONG :LAG1")), QStringLiteral("PONG :LAG1"));
        QCOMPARE(redactIrcLine(QString()), QString());

        const QString tagged =
            QStringLiteral("@time=2020-01-01T00:00:00.000Z :me!u@h PRIVMSG NickServ :IDENTIFY hunter2");
        const QString out = redactIrcLine(tagged);
        QVERIFY(out.startsWith(QStringLiteral("@time=2020-01-01T00:00:00.000Z ")));
        QVERIFY(out.contains(QStringLiteral("IDENTIFY ***")));
        QVERIFY(!out.contains(QStringLiteral("hunter2")));
    }

    void testOutgoingChat()
    {
        using CredentialRedactor::redactOutgoingChat;
        QCOMPARE(redactOutgoingChat(QStringLiteral("NickServ"), QStringLiteral("IDENTIFY hunter2")),
                 QStringLiteral("IDENTIFY ***"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("nickserv"),
                                    QStringLiteral("IDENTIFY account hunter2")),
                 QStringLiteral("IDENTIFY ***"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("ChanServ"),
                                    QStringLiteral("IDENTIFY #mychan hunter2")),
                 QStringLiteral("IDENTIFY ***"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("NickServ"),
                                    QStringLiteral("REGISTER hunter2 a@b.c")),
                 QStringLiteral("REGISTER ***"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("NickServ"),
                                    QStringLiteral("GHOST oldnick hunter2")),
                 QStringLiteral("GHOST ***"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("NickServ"),
                                    QStringLiteral("SET PASSWORD hunter2")),
                 QStringLiteral("SET PASSWORD ***"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("NickServ"),
                                    QStringLiteral("SET EMAIL a@b.c")),
                 QStringLiteral("SET EMAIL a@b.c"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("NickServ"), QStringLiteral("HELP")),
                 QStringLiteral("HELP"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("NickServ"), QStringLiteral("IDENTIFY")),
                 QStringLiteral("IDENTIFY"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("#chan"), QStringLiteral("IDENTIFY hunter2")),
                 QStringLiteral("IDENTIFY hunter2"));
        QCOMPARE(redactOutgoingChat(QStringLiteral("Q"), QStringLiteral("AUTH account hunter2")),
                 QStringLiteral("AUTH ***"));
        QVERIFY(CredentialRedactor::isServiceNick(
            QStringLiteral("NickServ@services.libera.chat")));
        QVERIFY(!CredentialRedactor::isServiceNick(QStringLiteral("#libera")));
    }

    void testDebugIsRedactedSignalIsNot()
    {
        QString captured;
        g_captured = &captured;
        qInstallMessageHandler(captureMessage);

        IrcConnectionTestable conn;
        QSignalSpy spy(&conn, &IrcConnection::rawLineReceived);
        conn.feedLine(QStringLiteral("PASS :s3cret-value"));
        conn.feedLine(QStringLiteral(":n!u@h PRIVMSG #c :hello there"));

        qInstallMessageHandler(nullptr);
        g_captured = nullptr;

        QVERIFY2(captured.contains(QStringLiteral("PASS ***")), qPrintable(captured));
        QVERIFY(!captured.contains(QStringLiteral("s3cret-value")));
        QVERIFY(captured.contains(QStringLiteral("hello there")));

        QCOMPARE(spy.count(), 2);
        // Scripts still see the original inbound line.
        QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("PASS :s3cret-value"));
    }
};

QTEST_MAIN(TestRedact)
#include "test_redact.moc"
