#include <QtTest>
#include "MessageParser.h"

class TestMessageParser : public QObject
{
    Q_OBJECT

private slots:
    // Non-command input is returned unchanged
    void testPlainText()
    {
        QCOMPARE(MessageParser::parse("hello"), QString("hello"));
        QCOMPARE(MessageParser::parse(""), QString(""));
        QCOMPARE(MessageParser::parse("hello world"), QString("hello world"));
    }

    // Known aliases are expanded to canonical uppercase commands
    void testAliasExpansion()
    {
        QCOMPARE(MessageParser::parse("/j #test"),    QString("/JOIN #test"));
        QCOMPARE(MessageParser::parse("/leave"),       QString("/PART"));
        QCOMPARE(MessageParser::parse("/p #chan"),     QString("/PART #chan"));
        QCOMPARE(MessageParser::parse("/w nick"),      QString("/WHOIS nick"));
        QCOMPARE(MessageParser::parse("/wi nick"),     QString("/WHOIS nick"));
        QCOMPARE(MessageParser::parse("/t new topic"), QString("/TOPIC new topic"));
        QCOMPARE(MessageParser::parse("/q"),           QString("/MSG"));
        QCOMPARE(MessageParser::parse("/m nick hi"),   QString("/MSG nick hi"));
        QCOMPARE(MessageParser::parse("/quote RAW"),   QString("/RAW RAW"));
        QCOMPARE(MessageParser::parse("/disconnect"),  QString("/QUIT"));
        QCOMPARE(MessageParser::parse("/bye reason"),  QString("/QUIT reason"));
        QCOMPARE(MessageParser::parse("/b nick"),      QString("/BAN nick"));
        QCOMPARE(MessageParser::parse("/inv nick"),    QString("/INVITE nick"));
        QCOMPARE(MessageParser::parse("/kickban n"),   QString("/KB n"));
    }

    // Unknown /commands are passed through unchanged
    void testUnknownCommandPassthrough()
    {
        QCOMPARE(MessageParser::parse("/join #foo"),  QString("/join #foo"));
        QCOMPARE(MessageParser::parse("/KICK nick"),  QString("/KICK nick"));
        QCOMPARE(MessageParser::parse("/unknown x"),  QString("/unknown x"));
    }

    // Case-insensitivity of the alias lookup (aliases are lowercase-matched)
    void testAliasCaseInsensitive()
    {
        QCOMPARE(MessageParser::parse("/J #test"),    QString("/JOIN #test"));
        QCOMPARE(MessageParser::parse("/W nick"),     QString("/WHOIS nick"));
        QCOMPARE(MessageParser::parse("/LEAVE"),      QString("/PART"));
    }

    // Alias with no arguments
    void testAliasNoArgs()
    {
        QCOMPARE(MessageParser::parse("/j"), QString("/JOIN"));
        QCOMPARE(MessageParser::parse("/w"), QString("/WHOIS"));
    }

    // Services shorthands
    void testServicesAliases()
    {
        QCOMPARE(MessageParser::parse("/nickserv identify pw"), QString("/NS identify pw"));
        QCOMPARE(MessageParser::parse("/chanserv op #c"),       QString("/CS op #c"));
    }
};

QTEST_MAIN(TestMessageParser)
#include "test_messageparser.moc"
