#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include "MessageModel.h"

class TestMessageModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName("NUchatTestMsg");
        QCoreApplication::setApplicationName("NUchatTestMsg");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_tempDir->path());
    }

    void testBackgroundAndHexColors()
    {
        const QString fgBg = MessageModel::ircToHtml(QStringLiteral("\x03" "04,08Hi"));
        QVERIFY2(fgBg.contains(QStringLiteral("color:#ff0000")),
                 qPrintable(fgBg));
        QVERIFY2(fgBg.contains(QStringLiteral("background-color:#ffff00")),
                 qPrintable(fgBg));
        QVERIFY(fgBg.contains(QStringLiteral("Hi")));

        const QString bgOnly = MessageModel::ircToHtml(QStringLiteral("\x03" ",02Bg"));
        QVERIFY2(bgOnly.contains(QStringLiteral("style=\"background-color:#00007f;\"")),
                 qPrintable(bgOnly));

        const QString hex =
            MessageModel::ircToHtml(QStringLiteral("\x04" "FF00AA,00CC00Z"));
        QVERIFY2(hex.contains(QStringLiteral("color:#FF00AA")), qPrintable(hex));
        QVERIFY2(hex.contains(QStringLiteral("background-color:#00CC00")),
                 qPrintable(hex));
    }

    void testReverseSwapsForegroundAndBackground()
    {
        MessageModel model;
        model.setDarkMode(true);
        const QString html =
            MessageModel::ircToHtml(QStringLiteral("\x03" "04X\x16" "Y"));
        QVERIFY2(html.contains(QStringLiteral("color:#ff0000")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("background-color:#ff0000")),
                 qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("color:#000000")), qPrintable(html));
    }

    void testHtmlTagsAreEscapedNotDeleted()
    {
        const QString html =
            MessageModel::ircToHtml(QStringLiteral("<code>keep</code> <a href=\"x\">"));
        QVERIFY(html.contains(QStringLiteral("&lt;code&gt;keep&lt;/code&gt;")));
        QVERIFY(html.contains(QStringLiteral("&lt;a href=&quot;x&quot;&gt;")));
        QVERIFY(!html.contains(QStringLiteral("<code>")));
        QVERIFY(!html.contains(QStringLiteral("<a ")));
    }

    void testStripMircColorsKeepsText()
    {
        setPref(QStringLiteral("ui/stripMircColors"), true);
        const QString html =
            MessageModel::ircToHtml(QStringLiteral("\x03" "04,08hi"));
        QVERIFY(!html.contains(QStringLiteral("color:")));
        QVERIFY(!html.contains(QStringLiteral("background-color:")));
        QVERIFY(html.contains(QStringLiteral("hi")));
        setPref(QStringLiteral("ui/stripMircColors"), false);
    }

    void testNickColorsAndClickableUrlsFollowPrefs()
    {
        MessageModel model;
        model.addMessage(QStringLiteral("chat"),
                         QStringLiteral("<alice> see https://example.com/a"));
        const QString colored = model.allFormattedText();
        QVERIFY(colored.contains(QStringLiteral("nick://alice")));
        QVERIFY(colored.contains(QStringLiteral("href=\"https://example.com/a\"")));

        setPref(QStringLiteral("ui/showNickColors"), false);
        setPref(QStringLiteral("url/clickable"), false);
        model.addMessage(QStringLiteral("chat"),
                         QStringLiteral("<bob> see https://example.com/b"));
        const QString plain = model.data(model.index(1), MessageModel::FormattedTextRole)
                                  .toString();
        QVERIFY(!plain.contains(QStringLiteral("nick://")));
        QVERIFY(!plain.contains(QStringLiteral("<a href=")));
        QVERIFY(plain.contains(QStringLiteral("https://example.com/b")));

        setPref(QStringLiteral("ui/showNickColors"), true);
        setPref(QStringLiteral("url/clickable"), true);
    }

    void testHighlightNickAndWords()
    {
        setPref(QStringLiteral("notify/highlightNick"), true);
        setPref(QStringLiteral("ui/highlightWords"), QStringLiteral("banana, melon"));
        MessageModel model;
        model.setNickname(QStringLiteral("Alice"));
        model.setHighlightEnabled(true);
        model.addMessage(QStringLiteral("chat"), QStringLiteral("<bob> hi Alice"));
        const QString nickHit =
            model.data(model.index(0), MessageModel::FormattedTextRole).toString();
        QVERIFY(nickHit.contains(QStringLiteral("background-color:#3a2a00")));

        model.addMessage(QStringLiteral("chat"), QStringLiteral("<bob> eat banana now"));
        const QString wordHit =
            model.data(model.index(1), MessageModel::FormattedTextRole).toString();
        QVERIFY(wordHit.contains(QStringLiteral("background-color:#3a2a00")));

        setPref(QStringLiteral("notify/highlightNick"), false);
        setPref(QStringLiteral("ui/highlightWords"), QString());
        model.addMessage(QStringLiteral("chat"), QStringLiteral("<bob> hi Alice"));
        const QString quiet =
            model.data(model.index(2), MessageModel::FormattedTextRole).toString();
        QVERIFY(!quiet.contains(QStringLiteral("background-color:#3a2a00")));
    }

    void testEmbedFollowsOriginBuffer()
    {
        MessageModel model;
        QSignalSpy spy(&model, &MessageModel::embedReady);
        model.setBufferContext(QStringLiteral("alpha"), QStringLiteral("#one"));

        MessageModel::PendingImage here;
        here.server = QStringLiteral("alpha");
        here.channel = QStringLiteral("#one");
        model.m_pendingImages.insert(QStringLiteral("http://img.test/a.png"), here);
        model.onImageReady(QStringLiteral("http://img.test/a.png"),
                           QStringLiteral("/tmp/a\"b.png"), 100, 50);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(spy.count(), 1);
        const QString shown =
            model.data(model.index(0), MessageModel::TextRole).toString();
        QVERIFY(shown.contains(QStringLiteral("file:///tmp/a&quot;b.png")));
        QVERIFY(!shown.contains(QStringLiteral("a\"b")));

        model.setBufferContext(QStringLiteral("beta"), QStringLiteral("#two"));
        model.clear();
        QCOMPARE(model.rowCount(), 0);

        MessageModel::PendingImage elsewhere;
        elsewhere.server = QStringLiteral("alpha");
        elsewhere.channel = QStringLiteral("#One");
        model.m_pendingImages.insert(QStringLiteral("http://img.test/b.png"),
                                     elsewhere);
        model.onImageReady(QStringLiteral("http://img.test/b.png"),
                           QStringLiteral("/tmp/b.png"), 80, 40);
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toString(), QStringLiteral("alpha"));
        QCOMPARE(spy.at(1).at(1).toString(), QStringLiteral("#One"));
        QVERIFY(spy.at(1).at(2).toString().contains(QStringLiteral("file:///tmp/b.png")));
    }

    void testMessageCountIsCappedAtScrollback()
    {
        setPref(QStringLiteral("ui/maxScrollback"), 100);
        MessageModel model;
        for (int i = 0; i < 130; ++i)
            model.addMessage(QStringLiteral("chat"),
                             QStringLiteral("<nick> line %1").arg(i));
        QCOMPARE(model.messageCount(), 100);
        QCOMPARE(model.rowCount(), 100);
        setPref(QStringLiteral("ui/maxScrollback"), 2000);
    }

private:
    void setPref(const QString &key, const QVariant &value)
    {
        QSettings prefs;
        prefs.setValue(key, value);
        prefs.sync();
    }

    QScopedPointer<QTemporaryDir> m_tempDir;
};

QTEST_MAIN(TestMessageModel)
#include "test_messagemodel.moc"
