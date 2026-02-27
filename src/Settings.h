#pragma once

#include <QObject>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = nullptr);
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void sync();
    Q_INVOKABLE QString configPath() const;

private:
    QSettings settings;
};
