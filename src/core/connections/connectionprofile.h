#pragma once

#include <QUuid>

struct ConnectionProfile
{
    QUuid id = QUuid::createUuid();
    QString name;
    QString host;
    quint16 port = 5432;
    QString database;
    QString user;
};
