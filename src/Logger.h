#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    void log(const QString &channel, const QString &message);
};
