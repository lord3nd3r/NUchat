#include <QtTest>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QTemporaryDir>
#include "Logger.h"

class TestLogger : public QObject
{
    Q_OBJECT

private:
    QString readFile(const QString &path) const
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(f.readAll());
    }

private slots:
    void init()
    {
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());
        m_logger.reset(new Logger());
        m_logger->setLogDirectory(m_tempDir->path() + QStringLiteral("/logs"));
    }

    void testStripsControlsAndIsReadableAfterReturn()
    {
        const QString raw = QStringLiteral("\x03") + QStringLiteral("04,02red")
            + QChar(0x0F) + QChar(0x02) + QStringLiteral("bold") + QChar(0x07)
            + QStringLiteral("bell\nsecond") + QStringLiteral("\x1B[31mansi");
        m_logger->log(QStringLiteral("net"), QStringLiteral("#chan"),
                      QStringLiteral("chat"), raw);

        const QString path = m_logger->logDir() + QStringLiteral("/net/#chan.log");
        const QString body = readFile(path);
        QVERIFY2(!body.isEmpty(), qPrintable(path));
        QCOMPARE(body.count(QLatin1Char('\n')), 1);
        QVERIFY(body.contains(QStringLiteral("redboldbell secondansi")));
        QVERIFY(!body.contains(QChar(0x03)));
        QVERIFY(!body.contains(QChar(0x1B)));
        QVERIFY(!body.contains(QChar(0x07)));
        QVERIFY(body.contains(QStringLiteral("[chat]")));

        const auto entries = m_logger->loadScrollback(QStringLiteral("net"),
                                                      QStringLiteral("#chan"), 10);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0).type, QStringLiteral("chat"));
        QVERIFY(entries.at(0).text.contains(QStringLiteral("redboldbell secondansi")));
        QCOMPARE(entries.at(0).timestamp.size(), 19);
    }

    void testPlainUnicodeIsPreserved()
    {
        m_logger->log(QStringLiteral("Net"), QStringLiteral("#Chan"),
                      QStringLiteral("chat"), QStringLiteral("héllo"));
        const QString body = readFile(m_logger->logDir() + QStringLiteral("/net/#chan.log"));
        QVERIFY(body.contains(QStringLiteral("héllo")));
    }

    void testOwnerOnlyPermissions()
    {
        m_logger->log(QStringLiteral("net"), QStringLiteral("chan"),
                      QStringLiteral("chat"), QStringLiteral("private"));
        const QString path = m_logger->logDir() + QStringLiteral("/net/chan.log");
        const auto perms = QFile::permissions(path);
        QVERIFY(perms & QFileDevice::ReadOwner);
        QVERIFY(!(perms & QFileDevice::ReadGroup));
        QVERIFY(!(perms & QFileDevice::ReadOther));
        QVERIFY(!(perms & QFileDevice::WriteOther));
    }

    void testRotationKeepsSeveralBackups()
    {
        m_logger->setRotationLimits(100, 2);
        const QString net = QStringLiteral("rot");
        const QString chan = QStringLiteral("c");
        auto line = [](QChar c) { return QString(80, c); };
        m_logger->log(net, chan, QStringLiteral("chat"), line('A'));
        m_logger->log(net, chan, QStringLiteral("chat"), line('B'));
        m_logger->log(net, chan, QStringLiteral("chat"), line('C'));
        m_logger->log(net, chan, QStringLiteral("chat"), line('D'));

        const QString base = m_logger->logDir() + QStringLiteral("/rot/c.log");
        QVERIFY(readFile(base).contains(QString(80, 'D')));
        QVERIFY(readFile(base + QStringLiteral(".1")).contains(QString(80, 'C')));
        QVERIFY(readFile(base + QStringLiteral(".2")).contains(QString(80, 'B')));
        QVERIFY(!QFile::exists(base + QStringLiteral(".3")));
        QVERIFY(!readFile(base + QStringLiteral(".2")).contains(QString(80, 'A')));
        QVERIFY(!readFile(base).contains(QString(80, 'A')));
    }

    void testDotDotNetworkDoesNotEscape()
    {
        const QString token = QStringLiteral("PATH-TOKEN-UNIQUE");
        m_logger->log(QStringLiteral(".."), QStringLiteral(".."),
                      QStringLiteral("chat"), token);
        const QString path = m_logger->logDir() + QStringLiteral("/_/_.log");
        QVERIFY2(QFile::exists(path), qPrintable(path));
        QVERIFY(readFile(path).contains(token));
        QVERIFY(!QFile::exists(m_tempDir->path() + QStringLiteral("/_.log")));
    }

private:
    QScopedPointer<QTemporaryDir> m_tempDir;
    QScopedPointer<Logger> m_logger;
};

QTEST_MAIN(TestLogger)
#include "test_logger.moc"
