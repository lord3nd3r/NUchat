#pragma once

#include <QObject>
#include <QVector>

class PluginInterface;
class IrcConnection;

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    void loadPlugins(const QString &directory);

public slots:
    void onMessage(IrcConnection *conn, const QString &msg);

private:
    QVector<PluginInterface*> plugins;
};
