#include "connectionstore.h"
#include <QList>
#include <QSettings>

ConnectionStore::ConnectionStore()
{
    QSettings settings;

    int size = settings.beginReadArray("connections");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        ConnectionProfile conn;
        const QUuid storedId = settings.value("id").toUuid();
        if (!storedId.isNull()) {
            conn.id = storedId;
        }
        conn.name = settings.value("name").toString();
        conn.database = settings.value("database").toString();
        conn.host = settings.value("host").toString();
        bool portOk = false;
        const int storedPort = settings.value("port", conn.port).toInt(&portOk);
        if (portOk && storedPort >= 1 && storedPort <= 65535) {
            conn.port = static_cast<quint16>(storedPort);
        }
        conn.user = settings.value("user").toString();
        m_connections.append(conn);
    }
    settings.endArray();
}

bool ConnectionStore::save() const
{
    QSettings settings;
    settings.beginWriteArray("connections", m_connections.size());
    for (int i = 0; i < m_connections.length(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("id", m_connections.at(i).id);
        settings.setValue("name", m_connections.at(i).name);
        settings.setValue("database", m_connections.at(i).database);
        settings.setValue("host", m_connections.at(i).host);
        settings.setValue("port", m_connections.at(i).port);
        settings.setValue("user", m_connections.at(i).user);
    }
    settings.endArray();
    settings.sync();
    return settings.status() == QSettings::NoError;
}

const QList<ConnectionProfile> &ConnectionStore::connections() const
{
    return m_connections;
}

void ConnectionStore::addConnection(const ConnectionProfile &conn)
{
    m_connections << conn;
}

void ConnectionStore::removeConnectionAt(int i)
{
    if (i >= 0 && i < m_connections.length()) {
        m_connections.removeAt(i);
    }
}
