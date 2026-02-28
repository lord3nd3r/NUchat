#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    void log(const QString &network, const QString &channel, const QString &message);
    QString logDir() const;
};
