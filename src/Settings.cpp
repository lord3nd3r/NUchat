#include "Settings.h"
#include "SecretStore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QUuid>
#include <QVariantList>
#include <QVariantMap>

Settings::Settings(QObject *parent)
    : QObject(parent), settings()
{
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue) const
{
    if (key == QLatin1String("networks/list")) {
        if (!settings.contains(key))
            return defaultValue;
        return const_cast<Settings *>(this)->hydrateNetworks(settings.value(key).toString());
    }
    if (key == QLatin1String("conn/proxyPass")) {
        if (!settings.contains(key)) {
            const QString migrated =
                const_cast<Settings *>(this)->hydrateProxyPass(QString());
            if (!migrated.isEmpty())
                return migrated;
            return defaultValue;
        }
        return const_cast<Settings *>(this)->hydrateProxyPass(settings.value(key).toString());
    }
    return settings.value(key, defaultValue);
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    if (key == QLatin1String("networks/list")) {
        storeNetworks(value.toString());
        return;
    }
    if (key == QLatin1String("conn/proxyPass")) {
        storeProxyPass(value.toString());
        return;
    }
    settings.setValue(key, value);
}

void Settings::remove(const QString &key)
{
    settings.remove(key);
}

void Settings::sync()
{
    settings.sync();
}

QStringList Settings::allKeysIn(const QString &group) const
{
    // const_cast: beginGroup/endGroup don't modify stored values
    QSettings &s = const_cast<QSettings &>(settings);
    s.beginGroup(group);
    QStringList keys = s.childKeys();
    s.endGroup();
    return keys;
}

QString Settings::configPath() const
{
    return QFileInfo(settings.fileName()).absolutePath();
}

// ── Type-safe accessors ──

int Settings::getInt(const QString &key, int defaultValue) const
{
    return settings.value(key, defaultValue).toInt();
}

bool Settings::getBool(const QString &key, bool defaultValue) const
{
    return settings.value(key, defaultValue).toBool();
}

QString Settings::getString(const QString &key, const QString &defaultValue) const
{
    if (key == QLatin1String("conn/proxyPass") || key == QLatin1String("networks/list"))
        return value(key, defaultValue).toString();
    return settings.value(key, defaultValue).toString();
}

double Settings::getDouble(const QString &key, double defaultValue) const
{
    return settings.value(key, defaultValue).toDouble();
}

void Settings::setInt(const QString &key, int v)
{
    settings.setValue(key, v);
}

void Settings::setBool(const QString &key, bool v)
{
    settings.setValue(key, v);
}

void Settings::setString(const QString &key, const QString &v)
{
    if (key == QLatin1String("networks/list") || key == QLatin1String("conn/proxyPass")) {
        setValue(key, v);
        return;
    }
    settings.setValue(key, v);
}

// ── Password fields ──
// server / SASL / NickServ / ZNC secrets live in the OS keychain when it is
// available. The JSON (and conn/proxyPass) keep every other field. A failed
// keychain write leaves the plaintext in place so nothing is dropped.

namespace {

const char *kSecretFields[] = {
    "saslPass", "nickServPass", "serverPass", "zncPass"
};
const QString kProxySecretKey = QStringLiteral("proxy.password");

QString secretKey(const QString &id, const char *field)
{
    return QStringLiteral("net.") + id + QLatin1Char('.') + QLatin1String(field);
}

bool acceptableId(const QString &id)
{
    if (id.isEmpty() || id.size() > 80)
        return false;
    for (const QChar &c : id) {
        if (c.isLetterOrNumber() || c == QLatin1Char('-') || c == QLatin1Char('_'))
            continue;
        return false;
    }
    return true;
}

QString makeSecretId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool parseNetworkArray(const QString &json, QJsonArray *out)
{
    if (json.trimmed().isEmpty()) {
        *out = QJsonArray();
        return true;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return false;
    *out = doc.array();
    return true;
}

QString arrayToJson(const QJsonArray &arr)
{
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QSet<QString> idsIn(const QJsonArray &arr)
{
    QSet<QString> ids;
    for (const QJsonValue &v : arr) {
        if (!v.isObject())
            continue;
        const QString id = v.toObject().value(QStringLiteral("secretId")).toString();
        if (acceptableId(id))
            ids.insert(id);
    }
    return ids;
}

bool objectHasPlainSecret(const QJsonObject &obj)
{
    for (const char *field : kSecretFields) {
        if (!obj.value(QLatin1String(field)).toString().isEmpty())
            return true;
    }
    return false;
}

void removeNetworkSecrets(const QString &id)
{
    SecretStore &store = SecretStore::instance();
    for (const char *field : kSecretFields)
        store.remove(secretKey(id, field));
}

bool secretsUnreadable(const QString &id)
{
    SecretStore &store = SecretStore::instance();
    for (const char *field : kSecretFields) {
        if (store.readFailed(secretKey(id, field)))
            return true;
    }
    return false;
}

} // namespace

QString Settings::hydrateNetworks(const QString &json)
{
    QJsonArray arr;
    if (!parseNetworkArray(json, &arr))
        return json;

    QJsonArray storedArr;
    QJsonArray returnedArr;
    bool dirty = false;
    SecretStore &store = SecretStore::instance();

    for (const QJsonValue &v : arr) {
        if (!v.isObject()) {
            storedArr.append(v);
            returnedArr.append(v);
            continue;
        }
        QJsonObject stored = v.toObject();
        QJsonObject returned = stored;
        QString id = stored.value(QStringLiteral("secretId")).toString();
        if (!acceptableId(id)) {
            if (objectHasPlainSecret(stored)) {
                id = makeSecretId();
                stored.insert(QStringLiteral("secretId"), id);
                returned.insert(QStringLiteral("secretId"), id);
                dirty = true;
            } else {
                id.clear();
            }
        }

        for (const char *field : kSecretFields) {
            const QString plain = stored.value(QLatin1String(field)).toString();
            const QString key = id.isEmpty() ? QString() : secretKey(id, field);
            if (!plain.isEmpty()) {
                // Keychain first, then drop the conf copy. A failed write
                // leaves `plain` in the stored object.
                if (!key.isEmpty() && store.write(key, plain)) {
                    stored.insert(QLatin1String(field), QString());
                    dirty = true;
                }
                returned.insert(QLatin1String(field), plain);
            } else if (!key.isEmpty()) {
                QString secret;
                if (store.read(key, &secret) == SecretStore::ReadStatus::Found)
                    returned.insert(QLatin1String(field), secret);
            }
        }
        storedArr.append(stored);
        returnedArr.append(returned);
    }

    if (dirty) {
        settings.setValue(QStringLiteral("networks/list"), arrayToJson(storedArr));
        settings.sync();
    }
    return arrayToJson(returnedArr);
}

void Settings::storeNetworks(const QString &json)
{
    QJsonArray incoming;
    if (!parseNetworkArray(json, &incoming)) {
        settings.setValue(QStringLiteral("networks/list"), json);
        settings.sync();
        return;
    }

    QJsonArray previous;
    parseNetworkArray(settings.value(QStringLiteral("networks/list")).toString(), &previous);
    const QSet<QString> oldIds = idsIn(previous);

    QJsonArray storedArr;
    QSet<QString> newIds;
    SecretStore &store = SecretStore::instance();

    for (const QJsonValue &v : incoming) {
        if (!v.isObject()) {
            storedArr.append(v);
            continue;
        }
        QJsonObject stored = v.toObject();
        QString id = stored.value(QStringLiteral("secretId")).toString();
        const bool needId = objectHasPlainSecret(stored) || acceptableId(id);
        if (needId && !acceptableId(id)) {
            id = makeSecretId();
            stored.insert(QStringLiteral("secretId"), id);
        }
        if (acceptableId(id))
            newIds.insert(id);

        for (const char *field : kSecretFields) {
            const QString plain = stored.value(QLatin1String(field)).toString();
            const QString key = acceptableId(id) ? secretKey(id, field) : QString();
            if (!plain.isEmpty()) {
                if (!key.isEmpty() && store.write(key, plain))
                    stored.insert(QLatin1String(field), QString());
                else
                    stored.insert(QLatin1String(field), plain);
            } else {
                // Empty means the user cleared it, unless we never managed
                // to load the previous secret (keychain read failed).
                if (!key.isEmpty() && !store.readFailed(key))
                    store.remove(key);
                if (stored.contains(QLatin1String(field)))
                    stored.insert(QLatin1String(field), QString());
            }
        }
        storedArr.append(stored);
    }

    for (const QString &oldId : oldIds) {
        if (newIds.contains(oldId))
            continue;
        if (secretsUnreadable(oldId))
            continue;
        removeNetworkSecrets(oldId);
    }

    settings.setValue(QStringLiteral("networks/list"), arrayToJson(storedArr));
    settings.sync();
}

QString Settings::hydrateProxyPass(const QString &stored)
{
    SecretStore &store = SecretStore::instance();
    if (!stored.isEmpty()) {
        if (store.write(kProxySecretKey, stored)) {
            settings.setValue(QStringLiteral("conn/proxyPass"), QString());
            settings.sync();
        }
        return stored;
    }
    QString secret;
    if (store.read(kProxySecretKey, &secret) == SecretStore::ReadStatus::Found)
        return secret;
    return stored;
}

void Settings::storeProxyPass(const QString &pass)
{
    SecretStore &store = SecretStore::instance();
    if (pass.isEmpty()) {
        if (!store.readFailed(kProxySecretKey))
            store.remove(kProxySecretKey);
        settings.setValue(QStringLiteral("conn/proxyPass"), QString());
    } else if (store.write(kProxySecretKey, pass)) {
        settings.setValue(QStringLiteral("conn/proxyPass"), QString());
    } else {
        settings.setValue(QStringLiteral("conn/proxyPass"), pass);
    }
    settings.sync();
}

void Settings::setDouble(const QString &key, double v)
{
    settings.setValue(key, v);
}

// ── HexChat migration ──

static QString hexchatAddonsDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/hexchat/addons";
}

bool Settings::hexchatScriptsExist() const
{
    QDir dir(hexchatAddonsDir());
    if (!dir.exists())
        return false;
    return !dir.entryList({"*.py", "*.lua"}, QDir::Files).isEmpty();
}

QStringList Settings::hexchatScriptFiles() const
{
    QDir dir(hexchatAddonsDir());
    if (!dir.exists())
        return {};
    QStringList result;
    for (const QString &f : dir.entryList({"*.py", "*.lua"}, QDir::Files))
        result << f;
    return result;
}

int Settings::importHexChatScripts() const
{
    QDir src(hexchatAddonsDir());
    if (!src.exists())
        return 0;

    QString destPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                       + "/NUchat/scripts";
    QDir destDir(destPath);
    if (!destDir.exists())
        destDir.mkpath(".");

    int count = 0;
    for (const QString &name : src.entryList({"*.py", "*.lua"}, QDir::Files)) {
        QString srcFile  = src.filePath(name);
        QString destFile = destDir.filePath(name);
        if (!QFile::exists(destFile)) {
            if (QFile::copy(srcFile, destFile))
                ++count;
        }
    }
    return count;
}

// ── HexChat config migration ──

static QString hexchatConfigDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/hexchat";
}

bool Settings::hexchatConfigExists() const
{
    return QFile::exists(hexchatConfigDir() + "/hexchat.conf")
        || QFile::exists(hexchatConfigDir() + "/servlist.conf");
}

QVariantMap Settings::importHexChatIdentity() const
{
    QVariantMap result;
    QFile f(hexchatConfigDir() + "/hexchat.conf");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed();
        QString val = line.mid(eq + 1).trimmed();
        if (key == "irc_nick1")     result["nick"]     = val;
        if (key == "irc_user_name") result["username"] = val;
        if (key == "irc_realname")  result["realname"] = val;
    }
    return result;
}

QVariantList Settings::importHexChatNetworks() const
{
    return parseHexChatServlist(hexchatConfigDir() + "/servlist.conf");
}

QVariantList Settings::parseHexChatServlist(const QString &path)
{
    QVariantList networks;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return networks;

    // Parse servlist.conf (HexChat format). Each network begins with an "N=..." line;
    // subsequent keys (S=, P=, I=, R=, J=, C=, F=, etc.) belong to that network until the next N= or EOF.
    // We flush the prior network when we encounter a new N= (key-driven, not blank-line based).
    QVariantMap current;
    QString firstName, firstServer;
    bool firstSsl = false;
    int firstPort = 6667;

    auto flushNetwork = [&]() {
        if (current.isEmpty() || !current.contains("network")) return;
        // Use first server entry
        if (!firstServer.isEmpty()) {
            current["server"] = firstServer;
            current["port"]   = firstPort;
            current["ssl"]    = firstSsl;
        }
        // Fill NUchat defaults for missing fields
        if (!current.contains("saslMethod"))  current["saslMethod"]  = QString("None");
        if (!current.contains("saslUser"))    current["saslUser"]    = QString();
        if (!current.contains("saslPass"))    current["saslPass"]    = QString();
        if (!current.contains("nickServCmd")) current["nickServCmd"] = QString();
        if (!current.contains("nickServPass"))current["nickServPass"]= QString();
        if (!current.contains("useGlobalNick"))current["useGlobalNick"] = true;
        if (!current.contains("customNick")) current["customNick"]   = QString();
        if (!current.contains("customUser")) current["customUser"]   = QString();
        if (!current.contains("customReal")) current["customReal"]   = QString();
        if (!current.contains("serverPass")) current["serverPass"]   = QString();
        if (!current.contains("autojoin"))   current["autojoin"]     = QString();
        if (!current.contains("perform"))    current["perform"]      = QString();
        current["isZnc"]     = false;
        current["zncUser"]   = QString();
        current["zncPass"]   = QString();
        current["zncNetwork"]= QString();
        networks.append(current);
    };

    auto startNetwork = [&]() {
        current.clear();
        firstServer.clear();
        firstName.clear();
        firstSsl = false;
        firstPort = 6667;
    };

    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine();
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed();
        QString val = line.mid(eq + 1).trimmed();

        if (key == "N") {
            if (!current.isEmpty() && current.contains("network")) {
                flushNetwork();
            }
            startNetwork();
            current["network"] = val;
        } else if (key == "S" && firstServer.isEmpty()) {
            // Server format: host/port  or  host/+port  or  host
            int slash = val.lastIndexOf('/');
            if (slash >= 0) {
                firstServer = val.left(slash);
                QString portStr = val.mid(slash + 1);
                if (portStr.startsWith('+')) {
                    firstSsl = true;
                    firstPort = portStr.mid(1).toInt();
                    if (firstPort == 0) firstPort = 6697;
                } else {
                    firstSsl = false;
                    firstPort = portStr.toInt();
                    if (firstPort == 0) firstPort = 6667;
                }
            } else {
                firstServer = val;
                firstSsl  = false;
                firstPort = 6667;
            }
        } else if (key == "P") {
            current["serverPass"] = val;
        } else if (key == "I") {
            // HexChat: I= is the per-network NICK (U= is the username)
            current["customNick"] = val;
            current["useGlobalNick"] = false;
        } else if (key == "U") {
            current["customUser"] = val;
            current["useGlobalNick"] = false;
        } else if (key == "R") {
            current["customReal"] = val;
            current["useGlobalNick"] = false;
        } else if (key == "J") {
            // Favorite/autojoin channel — HexChat writes one J= line per channel
            QString aj = current.value("autojoin").toString();
            current["autojoin"] = aj.isEmpty() ? val : aj + "," + val;
        } else if (key == "C") {
            // Connect command — one C= line per command
            QString pf = current.value("perform").toString();
            current["perform"] = pf.isEmpty() ? val : pf + "\n" + val;
        }
        // Other keys (E, F, D, L, A, B, v, etc.) are ignored for now.
    }
    flushNetwork(); // final network (no trailing N= after it)

    return networks;
}
