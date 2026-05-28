#include "Settings.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QVariantList>
#include <QVariantMap>

Settings::Settings(QObject *parent)
    : QObject(parent), settings()
{
}

QVariant Settings::value(const QString &key, const QVariant &defaultValue) const
{
    return settings.value(key, defaultValue);
}

void Settings::setValue(const QString &key, const QVariant &value)
{
    settings.setValue(key, value);
}

void Settings::sync()
{
    settings.sync();
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
    settings.setValue(key, v);
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
    QVariantList networks;
    QFile f(hexchatConfigDir() + "/servlist.conf");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return networks;

    // Parse each network block (blocks separated by blank lines)
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
        if (line.trimmed().isEmpty()) {
            flushNetwork();
            startNetwork();
            continue;
        }
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed();
        QString val = line.mid(eq + 1).trimmed();

        if (key == "N") {
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
            current["customUser"] = val;
            current["useGlobalNick"] = false;
        } else if (key == "R") {
            current["customReal"] = val;
        }
    }
    flushNetwork(); // handle last block without trailing blank line

    return networks;
}
