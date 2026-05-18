#include "Settings.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

Settings::Settings(QObject *parent)
    : QObject(parent), settings("NUchat", "NUchat")
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
