#include <QtTest>
#include <QTemporaryDir>
#include "Settings.h"

// Override the QSettings storage to an isolated temp directory so tests
// do not pollute the real user config.
class TestSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Use application-scoped QSettings backed by an in-memory path.
        QCoreApplication::setOrganizationName("NUchatTest");
        QCoreApplication::setApplicationName("NUchatTest");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        // Point the config to a temp dir so tests are isolated.
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_tempDir->path());
    }

    void testGetSetInt()
    {
        Settings s;
        QCOMPARE(s.getInt("test/int", 42), 42);   // default
        s.setInt("test/int", 7);
        QCOMPARE(s.getInt("test/int", 42), 7);
        s.setInt("test/int", -1);
        QCOMPARE(s.getInt("test/int", 0), -1);
    }

    void testGetSetBool()
    {
        Settings s;
        QCOMPARE(s.getBool("test/bool", true),  true);   // default
        QCOMPARE(s.getBool("test/bool2", false), false); // different default
        s.setBool("test/bool", false);
        QCOMPARE(s.getBool("test/bool", true), false);
        s.setBool("test/bool", true);
        QCOMPARE(s.getBool("test/bool", false), true);
    }

    void testGetSetString()
    {
        Settings s;
        QCOMPARE(s.getString("test/str", "hello"), QString("hello")); // default
        s.setString("test/str", "world");
        QCOMPARE(s.getString("test/str", "hello"), QString("world"));
        s.setString("test/str", "");
        QCOMPARE(s.getString("test/str", "fallback"), QString(""));
    }

    void testGetSetDouble()
    {
        Settings s;
        QCOMPARE(s.getDouble("test/d", 1.5), 1.5); // default
        s.setDouble("test/d", 3.14);
        QVERIFY(qAbs(s.getDouble("test/d", 0.0) - 3.14) < 1e-9);
    }

    void testGenericValueRoundtrip()
    {
        // The generic value() / setValue() interface still works
        Settings s;
        s.setValue("test/generic", QVariant(99));
        QCOMPARE(s.value("test/generic", 0).toInt(), 99);
    }

    void testSyncDoesNotCrash()
    {
        Settings s;
        s.setInt("test/synckey", 1);
        s.sync(); // must not crash
    }

    void testConfigPathIsNonEmpty()
    {
        Settings s;
        QVERIFY(!s.configPath().isEmpty());
    }

private:
    QScopedPointer<QTemporaryDir> m_tempDir;
};

QTEST_MAIN(TestSettings)
#include "test_settings.moc"
