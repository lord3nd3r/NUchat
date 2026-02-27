#pragma once

#include <QObject>
#include <QVariantMap>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap currentTheme READ currentTheme NOTIFY themeChanged)
public:
    explicit ThemeManager(QObject *parent = nullptr);

    QVariantMap currentTheme() const;
    Q_INVOKABLE bool loadTheme(const QString &filePath);
    Q_INVOKABLE QStringList availableThemes() const;

signals:
    void themeChanged();

private:
    QVariantMap m_theme;
};
