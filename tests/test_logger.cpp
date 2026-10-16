#include <QtTest>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include "Logger.h"
#include "Settings.h"
#include "UserPaths.h"

class TestLogger : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName("NUchatTestLog");
        QCoreApplication::setApplicationName("NUchatTestLog");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_tempDir->path());
    }

    void testExpandHomePath()
    {
        const QString downloads =
            expandHomePath(QStringLiteral("~/Downloads"));
        QCOMPARE(downloads, QDir::cleanPath(QDir::homePath() +
                                            QStringLiteral("/Downloads")));
        QCOMPARE(expandHomePath(QStringLiteral("~")),
                 QDir::cleanPath(QDir::homePath()));
        QCOMPARE(expandHomePath(QStringLiteral("/tmp/already")),
                 QStringLiteral("/tmp/already"));
        QCOMPARE(expandHomePath(QStringLiteral("~nosuchuserzz/x")),
                 QStringLiteral("~nosuchuserzz/x"));
    }

    void testDccSizeAndDownloadDir()
    {
        Settings s;
        s.remove(QStringLiteral("dcc/maxSize"));
        s.remove(QStringLiteral("dcc/maxFileSize"));
        s.remove(QStringLiteral("dcc/downloadDir"));
        QCOMPARE(dccMaxFileBytes(&s), 100LL * 1024 * 1024);

        s.setInt(QStringLiteral("dcc/maxSize"), 50);
        QCOMPARE(dccMaxFileBytes(&s), 50LL * 1024 * 1024);
        s.setInt(QStringLiteral("dcc/maxSize"), 0);
        QCOMPARE(dccMaxFileBytes(&s), 0);

        s.remove(QStringLiteral("dcc/maxSize"));
        s.setValue(QStringLiteral("dcc/maxFileSize"), 1234);
        QCOMPARE(dccMaxFileBytes(&s), 1234);

        s.setString(QStringLiteral("dcc/downloadDir"), QStringLiteral("~/Downloads"));
        QCOMPARE(dccDownloadDirectory(&s),
                 QDir::cleanPath(QDir::homePath() + QStringLiteral("/Downloads")));
    }

    void testDotDotStaysInsideLogRoot()
    {
        Settings s;
        s.setBool(QStringLiteral("log/enable"), true);
        s.setBool(QStringLiteral("log/timestamps"), true);
        s.setBool(QStringLiteral("log/perChannel"), true);
        s.setBool(QStringLiteral("log/privateMessages"), true);
        const QString root = m_tempDir->filePath(QStringLiteral("logs-dotdot"));
        s.setString(QStringLiteral("log/directory"), root);

        Logger logger;
        logger.setSettings(&s);
        logger.log(QStringLiteral(".."), QStringLiteral("#chan"),
                   QStringLiteral("chat"), QStringLiteral("secret-dot"));
        logger.log(QStringLiteral("../../etc"), QStringLiteral(".."),
                   QStringLiteral("chat"), QStringLiteral("secret-etc"));

        QVERIFY(!QFile::exists(QDir(root).absoluteFilePath(
            QStringLiteral("../#chan.log"))));
        QVERIFY(!QFile::exists(QDir(root).absoluteFilePath(
            QStringLiteral("../../etc.log"))));

        const QString dotLog =
            QDir(root).filePath(QStringLiteral("_../#chan.log"));
        const QString etcLog =
            QDir(root).filePath(QStringLiteral(".._.._etc/_...log"));
        QVERIFY2(QFile::exists(dotLog), qPrintable(dotLog));
        QVERIFY2(QFile::exists(etcLog), qPrintable(etcLog));

        const QString canonRoot = QFileInfo(root).canonicalFilePath();
        QDirIterator it(root, QStringList() << QStringLiteral("*.log"),
                        QDir::Files | QDir::Hidden,
                        QDirIterator::Subdirectories);
        QStringList files;
        while (it.hasNext())
            files << it.next();
        QCOMPARE(files.size(), 2);
        bool sawDot = false;
        bool sawEtc = false;
        for (const QString &path : files) {
            const QString canon = QFileInfo(path).canonicalFilePath();
            QVERIFY2(canon.startsWith(canonRoot + QLatin1Char('/')),
                     qPrintable(canon));
            const QString dirName = QFileInfo(path).dir().dirName();
            QVERIFY(dirName != QLatin1String(".."));
            QVERIFY(dirName != QLatin1String("."));
            if (dirName == QLatin1String("_.."))
                sawDot = true;
            if (dirName == QLatin1String(".._.._etc"))
                sawEtc = true;
            const QFileDevice::Permissions perms = QFile::permissions(path);
            QVERIFY(!(perms & (QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                               QFileDevice::ExeGroup | QFileDevice::ReadOther |
                               QFileDevice::WriteOther | QFileDevice::ExeOther)));
        }
        QVERIFY(sawDot);
        QVERIFY(sawEtc);
    }

    void testLoggingPrefs()
    {
        Settings s;
        const QString root = m_tempDir->filePath(QStringLiteral("logs-prefs"));
        s.setString(QStringLiteral("log/directory"), root);
        s.setBool(QStringLiteral("log/enable"), false);
        s.setBool(QStringLiteral("log/timestamps"), true);
        s.setBool(QStringLiteral("log/perChannel"), true);
        s.setBool(QStringLiteral("log/privateMessages"), true);

        Logger logger;
        logger.setSettings(&s);
        logger.log(QStringLiteral("net"), QStringLiteral("#c"),
                   QStringLiteral("chat"), QStringLiteral("hidden"));
        QVERIFY(logger.loadScrollback(QStringLiteral("net"), QStringLiteral("#c"))
                    .isEmpty());

        s.setBool(QStringLiteral("log/enable"), true);
        s.setBool(QStringLiteral("log/timestamps"), false);
        logger.log(QStringLiteral("net"), QStringLiteral("#c"),
                   QStringLiteral("chat"), QStringLiteral("hello ts"));
        const auto plain =
            logger.loadScrollback(QStringLiteral("net"), QStringLiteral("#c"));
        QCOMPARE(plain.size(), 1);
        QCOMPARE(plain[0].type, QStringLiteral("chat"));
        QCOMPARE(plain[0].text, QStringLiteral("hello ts"));
        QVERIFY(plain[0].timestamp.isEmpty());

        QFile raw(QDir(logger.logDir())
                      .filePath(QStringLiteral("net/#c.log")));
        QVERIFY(raw.open(QIODevice::ReadOnly));
        const QString line = QString::fromUtf8(raw.readAll()).trimmed();
        QCOMPARE(line, QStringLiteral("[chat] hello ts"));

        s.setBool(QStringLiteral("log/timestamps"), true);
        s.setBool(QStringLiteral("log/perChannel"), false);
        logger.log(QStringLiteral("sharednet"), QStringLiteral("#a"),
                   QStringLiteral("chat"), QStringLiteral("from-a"));
        logger.log(QStringLiteral("sharednet"), QStringLiteral("#b"),
                   QStringLiteral("chat"), QStringLiteral("from-b"));
        const auto fromA =
            logger.loadScrollback(QStringLiteral("sharednet"), QStringLiteral("#a"));
        const auto fromB =
            logger.loadScrollback(QStringLiteral("sharednet"), QStringLiteral("#b"));
        QCOMPARE(fromA.size(), 2);
        QCOMPARE(fromB.size(), 2);
        QCOMPARE(fromA[0].text, QStringLiteral("from-a"));
        QCOMPARE(fromA[1].text, QStringLiteral("from-b"));

        s.setBool(QStringLiteral("log/perChannel"), true);
        s.setBool(QStringLiteral("log/privateMessages"), false);
        logger.log(QStringLiteral("ircnet"), QStringLiteral("bob"),
                   QStringLiteral("chat"), QStringLiteral("pm-text"));
        logger.log(QStringLiteral("ircnet"), QStringLiteral("#pub"),
                   QStringLiteral("chat"), QStringLiteral("public-text"));
        logger.log(QStringLiteral("ircnet"), QStringLiteral("ircnet"),
                   QStringLiteral("system"), QStringLiteral("server-text"));
        QVERIFY(logger.loadScrollback(QStringLiteral("ircnet"), QStringLiteral("bob"))
                    .isEmpty());
        const auto pub =
            logger.loadScrollback(QStringLiteral("ircnet"), QStringLiteral("#pub"));
        QCOMPARE(pub.size(), 1);
        QCOMPARE(pub[0].text, QStringLiteral("public-text"));
        const auto server =
            logger.loadScrollback(QStringLiteral("ircnet"), QStringLiteral("ircnet"));
        QCOMPARE(server.size(), 1);
        QCOMPARE(server[0].text, QStringLiteral("server-text"));
    }

private:
    QScopedPointer<QTemporaryDir> m_tempDir;
};

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
