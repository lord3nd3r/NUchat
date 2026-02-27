#pragma once

#include <QObject>

class IrcConnection;

class PluginInterface
{
public:
    virtual ~PluginInterface() = default;
    virtual void initialize(QObject *parent) = 0;
    // called when a message arrives on a channel; return true if handled
    virtual bool handleCommand(const QString &command, const QStringList &args, IrcConnection *connection) { Q_UNUSED(command); Q_UNUSED(args); Q_UNUSED(connection); return false; }
};

#define PluginInterface_iid "com.nuchat.PluginInterface"

Q_DECLARE_INTERFACE(PluginInterface, PluginInterface_iid)
