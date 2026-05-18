#pragma once

#include <QObject>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = nullptr);

    // Generic accessor (used from QML)
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void sync();
    Q_INVOKABLE QString configPath() const;

    // HexChat script migration
    Q_INVOKABLE bool hexchatScriptsExist() const;
    Q_INVOKABLE QStringList hexchatScriptFiles() const;
    Q_INVOKABLE int importHexChatScripts() const;

    // Type-safe C++ accessors — preferred over value() in C++ code.
    int    getInt   (const QString &key, int           defaultValue = 0)     const;
    bool   getBool  (const QString &key, bool          defaultValue = false)  const;
    QString getString(const QString &key, const QString &defaultValue = {})  const;
    double getDouble(const QString &key, double         defaultValue = 0.0)  const;

    void setInt   (const QString &key, int            v);
    void setBool  (const QString &key, bool           v);
    void setString(const QString &key, const QString &v);
    void setDouble(const QString &key, double         v);

private:
    QSettings settings;
};
