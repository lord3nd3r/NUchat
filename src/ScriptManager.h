#pragma once

#include <QObject>
#include <QFileSystemWatcher>

#if __has_include(<QJSEngine>)
#include <QJSEngine>
#define HAVE_QJSE 1
#else
#define HAVE_QJSE 0
#endif

class IrcConnection;

class ScriptManager : public QObject
{
    Q_OBJECT
public:
    explicit ScriptManager(QObject *parent = nullptr);
    ~ScriptManager();
    void loadScripts(const QString &directory);

    void handleMessage(IrcConnection *conn, const QString &sender, const QString &message);

private slots:
    void onFileChanged(const QString &path);

private:
#if HAVE_QJSE
    QJSEngine m_engine;
#endif
    QString m_directory;
};
