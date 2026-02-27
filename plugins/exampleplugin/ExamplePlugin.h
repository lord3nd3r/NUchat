#pragma once

#include <QObject>
#include "PluginInterface.h"

class IrcConnection;

class ExamplePlugin : public QObject, public PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface)

public:
    void initialize(QObject *parent) override;
    bool handleCommand(const QString &command, const QStringList &args, IrcConnection *connection) override;
};
