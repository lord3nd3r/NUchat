#include "ThemeManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QStringList>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
}

QVariantMap ThemeManager::currentTheme() const
{
    return m_theme;
}

bool ThemeManager::loadTheme(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open theme" << filePath;
        return false;
    }
    QByteArray data = f.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return false;
    m_theme = doc.object().toVariantMap();
    emit themeChanged();
    return true;
}

QStringList ThemeManager::availableThemes() const
{
    QStringList names;
    // list theme files in qrc resources (alias prefix is /themes/)
    QDir dir(":/themes");
    for (const QString &file : dir.entryList({"*.json"}, QDir::Files)) {
        names << file;
    }
    return names;
}
