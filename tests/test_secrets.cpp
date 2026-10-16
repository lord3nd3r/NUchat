#include <QtTest>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include "Settings.h"
#include "SecretStore.h"

// Isolated QSettings plus an in-memory keychain, so tests never touch the
// real NUchat.conf or the OS keychain.
class TestSecrets : public QObject
{
    Q_OBJECT

private:
    QString configFileText() const
    {
        QSettings probe;
        QFile f(probe.fileName());
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(f.readAll());
    }

    QString rawNetworks() const
    {
        QSettings raw;
        raw.sync();
        return raw.value(QStringLiteral("networks/list")).toString();
    }

    static QString sampleJson()
    {
        return QStringLiteral(
            "[{"
            "\"network\":\"Libera\","
            "\"server\":\"irc.libera.chat\","
            "\"port\":6697,"
            "\"ssl\":true,"
            "\"saslMethod\":\"PLAIN\","
            "\"saslUser\":\"alice\","
            "\"saslPass\":\"SASL-SECRET\","
            "\"nickServPass\":\"NS-SECRET\","
            "\"serverPass\":\"SERVER-SECRET\","
            "\"zncPass\":\"ZNC-SECRET\","
            "\"zncUser\":\"zncuser\","
            "\"autojoin\":\"#nuchat\","
            "\"perform\":\"/msg NickServ IDENTIFY PERFORM-SECRET\","
            "\"customNote\":\"keep-me\""
            "}]");
    }

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("NUchatSecretTest"));
        QCoreApplication::setApplicationName(QStringLiteral("NUchatSecretTest"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        m_tempDir.reset(new QTemporaryDir());
        QVERIFY(m_tempDir->isValid());
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_tempDir->path());
        SecretStore::setModeForTests(SecretStore::Mode::InMemory);
    }

    void cleanup()
    {
        SecretStore::setModeForTests(SecretStore::Mode::InMemory);
        Settings s;
        s.remove(QStringLiteral("networks/list"));
        s.remove(QStringLiteral("conn/proxyPass"));
        s.remove(QStringLiteral("ui/theme"));
        s.sync();
    }

    void testMigrationStripsSecretsAndKeepsTheRest()
    {
        {
            QSettings seed;
            seed.setValue(QStringLiteral("networks/list"), sampleJson());
            seed.sync();
        }

        Settings s;
        const QString hydrated = s.value(QStringLiteral("networks/list")).toString();
        QVERIFY2(hydrated.contains(QStringLiteral("SASL-SECRET")),
                 qPrintable(hydrated));
        QVERIFY(hydrated.contains(QStringLiteral("NS-SECRET")));
        QVERIFY(hydrated.contains(QStringLiteral("SERVER-SECRET")));
        QVERIFY(hydrated.contains(QStringLiteral("ZNC-SECRET")));
        QVERIFY(hydrated.contains(QStringLiteral("PERFORM-SECRET")));
        QVERIFY(hydrated.contains(QStringLiteral("keep-me")));
        QVERIFY(hydrated.contains(QStringLiteral("alice")));
        QVERIFY(hydrated.contains(QStringLiteral("#nuchat")));

        const QString onDisk = configFileText();
        QVERIFY(!onDisk.contains(QStringLiteral("SASL-SECRET")));
        QVERIFY(!onDisk.contains(QStringLiteral("NS-SECRET")));
        QVERIFY(!onDisk.contains(QStringLiteral("SERVER-SECRET")));
        QVERIFY(!onDisk.contains(QStringLiteral("ZNC-SECRET")));
        // Free-form perform text is not rewritten.
        QVERIFY(onDisk.contains(QStringLiteral("PERFORM-SECRET")));
        QVERIFY(onDisk.contains(QStringLiteral("keep-me")));
        QVERIFY(onDisk.contains(QStringLiteral("irc.libera.chat")));

        const auto arr = QJsonDocument::fromJson(rawNetworks().toUtf8()).array();
        QCOMPARE(arr.size(), 1);
        const QString id = arr.at(0).toObject().value(QStringLiteral("secretId")).toString();
        QVERIFY(id.size() >= 8);

        QString sasl;
        QCOMPARE(SecretStore::instance().read(QStringLiteral("net.%1.saslPass").arg(id), &sasl),
                 SecretStore::ReadStatus::Found);
        QCOMPARE(sasl, QStringLiteral("SASL-SECRET"));
        QString ns;
        QVERIFY(SecretStore::instance().read(QStringLiteral("net.%1.nickServPass").arg(id), &ns)
                == SecretStore::ReadStatus::Found);
        QCOMPARE(ns, QStringLiteral("NS-SECRET"));
    }

    void testSecondInstanceReadsKeychainNotTheFile()
    {
        {
            Settings s;
            s.setValue(QStringLiteral("networks/list"), sampleJson());
        }
        Settings again;
        const QString hydrated = again.value(QStringLiteral("networks/list")).toString();
        QVERIFY(hydrated.contains(QStringLiteral("SASL-SECRET")));
        QVERIFY(!configFileText().contains(QStringLiteral("SASL-SECRET")));

        const auto first = QJsonDocument::fromJson(rawNetworks().toUtf8()).array();
        const QString id = first.at(0).toObject().value(QStringLiteral("secretId")).toString();
        const QString second = again.value(QStringLiteral("networks/list")).toString();
        const auto arr2 = QJsonDocument::fromJson(second.toUtf8()).array();
        QCOMPARE(arr2.at(0).toObject().value(QStringLiteral("secretId")).toString(), id);
    }

    void testUnicodeAndQuotesRoundTrip()
    {
        const QString secret = QStringLiteral("pä\"ss\\wörd");
        QJsonObject obj;
        obj.insert(QStringLiteral("network"), QStringLiteral("N"));
        obj.insert(QStringLiteral("server"), QStringLiteral("irc.example"));
        obj.insert(QStringLiteral("saslPass"), secret);
        obj.insert(QStringLiteral("port"), 6667);
        QJsonArray arr;
        arr.append(obj);
        const QString encoded = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));

        Settings s;
        s.setValue(QStringLiteral("networks/list"), encoded);
        QVERIFY(!configFileText().contains(secret));

        const auto back = QJsonDocument::fromJson(
                              s.value(QStringLiteral("networks/list")).toString().toUtf8())
                              .array();
        QCOMPARE(back.at(0).toObject().value(QStringLiteral("saslPass")).toString(), secret);
        QCOMPARE(back.at(0).toObject().value(QStringLiteral("port")).toInt(), 6667);
        QCOMPARE(back.at(0).toObject().value(QStringLiteral("server")).toString(),
                 QStringLiteral("irc.example"));
    }

    void testClearingPasswordDeletesTheSecret()
    {
        Settings s;
        s.setValue(QStringLiteral("networks/list"), sampleJson());
        const auto hydrated = QJsonDocument::fromJson(
                                  s.value(QStringLiteral("networks/list")).toString().toUtf8())
                                  .array();
        QJsonObject obj = hydrated.at(0).toObject();
        const QString id = obj.value(QStringLiteral("secretId")).toString();
        obj.insert(QStringLiteral("saslPass"), QString());
        QJsonArray arr;
        arr.append(obj);
        s.setValue(QStringLiteral("networks/list"),
                   QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));

        QString leftover;
        QCOMPARE(SecretStore::instance().read(QStringLiteral("net.%1.saslPass").arg(id), &leftover),
                 SecretStore::ReadStatus::Missing);
        // The other secrets on that network stay.
        QString ns;
        QCOMPARE(SecretStore::instance().read(QStringLiteral("net.%1.nickServPass").arg(id), &ns),
                 SecretStore::ReadStatus::Found);
        QCOMPARE(ns, QStringLiteral("NS-SECRET"));
    }

    void testRemovingANetworkRemovesItsSecrets()
    {
        QJsonArray arr;
        QJsonObject a;
        a.insert(QStringLiteral("network"), QStringLiteral("A"));
        a.insert(QStringLiteral("server"), QStringLiteral("a.example"));
        a.insert(QStringLiteral("saslPass"), QStringLiteral("SECRET-A"));
        QJsonObject b;
        b.insert(QStringLiteral("network"), QStringLiteral("B"));
        b.insert(QStringLiteral("server"), QStringLiteral("b.example"));
        b.insert(QStringLiteral("saslPass"), QStringLiteral("SECRET-B"));
        arr.append(a);
        arr.append(b);

        Settings s;
        s.setValue(QStringLiteral("networks/list"),
                   QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
        const auto hydrated = QJsonDocument::fromJson(
                                  s.value(QStringLiteral("networks/list")).toString().toUtf8())
                                  .array();
        QCOMPARE(hydrated.size(), 2);
        const QString idA = hydrated.at(0).toObject().value(QStringLiteral("secretId")).toString();
        const QString idB = hydrated.at(1).toObject().value(QStringLiteral("secretId")).toString();

        QJsonArray onlyB;
        onlyB.append(hydrated.at(1).toObject());
        s.setValue(QStringLiteral("networks/list"),
                   QString::fromUtf8(QJsonDocument(onlyB).toJson(QJsonDocument::Compact)));

        QString gone;
        QCOMPARE(SecretStore::instance().read(QStringLiteral("net.%1.saslPass").arg(idA), &gone),
                 SecretStore::ReadStatus::Missing);
        QString kept;
        QCOMPARE(SecretStore::instance().read(QStringLiteral("net.%1.saslPass").arg(idB), &kept),
                 SecretStore::ReadStatus::Found);
        QCOMPARE(kept, QStringLiteral("SECRET-B"));
        QVERIFY(configFileText().contains(QStringLiteral("b.example")));
        QVERIFY(!configFileText().contains(QStringLiteral("SECRET-B")));
    }

    void testNoPasswordsLeavesTheFileAlone()
    {
        const QString original = QStringLiteral(
            "[{\"network\":\"Plain\",\"server\":\"irc.example\",\"port\":6667,\"autojoin\":\"#x\"}]");
        {
            QSettings seed;
            seed.setValue(QStringLiteral("networks/list"), original);
            seed.sync();
        }
        Settings s;
        const QString hydrated = s.value(QStringLiteral("networks/list")).toString();
        QVERIFY(hydrated.contains(QStringLiteral("Plain")));
        QVERIFY(hydrated.contains(QStringLiteral("#x")));
        QCOMPARE(rawNetworks(), original);
    }

    void testInvalidJsonIsKeptVerbatim()
    {
        const QString broken = QStringLiteral("not-json { SASL-SECRET");
        Settings s;
        s.setValue(QStringLiteral("networks/list"), broken);
        QCOMPARE(rawNetworks(), broken);
        QCOMPARE(s.value(QStringLiteral("networks/list")).toString(), broken);
    }

    void testProxyPassword()
    {
        Settings s;
        s.setValue(QStringLiteral("conn/proxyPass"), QStringLiteral("PROXY-SECRET"));
        QVERIFY(!configFileText().contains(QStringLiteral("PROXY-SECRET")));
        QCOMPARE(s.getString(QStringLiteral("conn/proxyPass")), QStringLiteral("PROXY-SECRET"));
        QCOMPARE(s.value(QStringLiteral("conn/proxyPass")).toString(), QStringLiteral("PROXY-SECRET"));

        Settings again;
        QCOMPARE(again.getString(QStringLiteral("conn/proxyPass")), QStringLiteral("PROXY-SECRET"));

        again.setValue(QStringLiteral("conn/proxyPass"), QString());
        QCOMPARE(again.getString(QStringLiteral("conn/proxyPass"), QStringLiteral("nope")), QString());
        QString leftover;
        QCOMPARE(SecretStore::instance().read(QStringLiteral("proxy.password"), &leftover),
                 SecretStore::ReadStatus::Missing);
    }

    void testUnavailableKeychainKeepsPlaintext()
    {
        SecretStore::setModeForTests(SecretStore::Mode::Unavailable);
        Settings s;
        s.setValue(QStringLiteral("networks/list"), sampleJson());
        s.setValue(QStringLiteral("conn/proxyPass"), QStringLiteral("PROXY-SECRET"));

        const QString onDisk = configFileText();
        QVERIFY(onDisk.contains(QStringLiteral("SASL-SECRET")));
        QVERIFY(onDisk.contains(QStringLiteral("NS-SECRET")));
        QVERIFY(onDisk.contains(QStringLiteral("SERVER-SECRET")));
        QVERIFY(onDisk.contains(QStringLiteral("ZNC-SECRET")));
        QVERIFY(onDisk.contains(QStringLiteral("PROXY-SECRET")));
        QVERIFY(onDisk.contains(QStringLiteral("PERFORM-SECRET")));
        QVERIFY(onDisk.contains(QStringLiteral("irc.libera.chat")));

        const QString hydrated = s.value(QStringLiteral("networks/list")).toString();
        QVERIFY(hydrated.contains(QStringLiteral("SASL-SECRET")));
        QCOMPARE(s.getString(QStringLiteral("conn/proxyPass")), QStringLiteral("PROXY-SECRET"));
        QVERIFY(!SecretStore::instance().isSecure());
    }

    void testOtherSettingsAreUntouched()
    {
        Settings s;
        s.setValue(QStringLiteral("ui/theme"), QStringLiteral("nord"));
        QCOMPARE(s.value(QStringLiteral("ui/theme")).toString(), QStringLiteral("nord"));
        QCOMPARE(s.getString(QStringLiteral("ui/theme")), QStringLiteral("nord"));
    }

private:
    QScopedPointer<QTemporaryDir> m_tempDir;
};

QTEST_MAIN(TestSecrets)
#include "test_secrets.moc"
