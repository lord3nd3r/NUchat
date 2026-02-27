#include <QtTest>
#include "MessageParser.h"

class TestMessageParser : public QObject
{
    Q_OBJECT

private slots:
    void testParse()
    {
        QCOMPARE(MessageParser::parse("hello"), QString("hello"));
    }
};

QTEST_MAIN(TestMessageParser)
#include "test_messageparser.moc"
