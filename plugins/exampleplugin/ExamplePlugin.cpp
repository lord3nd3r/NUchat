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
    if (command == "hello") {
        if (connection) {
            connection->sendMessage("#general", "Hello from plugin!");
        }
        return true;
    }
    return false;
}
