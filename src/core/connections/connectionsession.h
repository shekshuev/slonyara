#pragma once

#include "connectionprofile.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

// The PostgreSQL connection is created, retained and destroyed in run().
class ConnectionSession final : public QThread
{
    Q_OBJECT
public:
    ConnectionSession(ConnectionProfile profile, QString password);
    ~ConnectionSession() override;
    const ConnectionProfile &profile() const { return m_profile; }
    void stop();

signals:
    void opened();
    void failed(const QString &details);

private:
    void run() override;
    ConnectionProfile m_profile;
    QString m_password;
    QMutex m_mutex;
    QWaitCondition m_stopped;
};
