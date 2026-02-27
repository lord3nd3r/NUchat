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
