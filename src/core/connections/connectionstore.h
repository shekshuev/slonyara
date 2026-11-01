#pragma once

#include <QList>
#include "connectionprofile.h"

class ConnectionStore
{
public:
    ConnectionStore();
    const QList<ConnectionProfile> &connections() const;
    bool save() const;
    void addConnection(const ConnectionProfile &conn);
    void removeConnectionAt(int i);

private:
    QList<ConnectionProfile> m_connections;
};
