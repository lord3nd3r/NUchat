#include "Settings.h"
#include <QFileInfo>

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
