#include "Logger.h"

Logger::Logger(QObject *parent)
    : QObject(parent)
{
}

void Logger::log(const QString &channel, const QString &message)
{
    Q_UNUSED(channel)
    Q_UNUSED(message)
    // stub: append to file per-channel
}
