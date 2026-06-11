#include "ExamplePlugin.h"
#include "IrcConnection.h"
#include <QDebug>

void ExamplePlugin::initialize(QObject *parent)
{
    Q_UNUSED(parent)
    qDebug() << "ExamplePlugin loaded";
}

bool ExamplePlugin::handleCommand(const QString &command, const QStringList &args, IrcConnection *connection)
{
    Q_UNUSED(args)
    if (command == "hello") {
        if (connection) {
            QMetaObject::invokeMethod(connection, "sendMessage",
                                      Q_ARG(QString, "#general"),
                                      Q_ARG(QString, "Hello from plugin!"));
        }
        return true;
    }
    return false;
}
