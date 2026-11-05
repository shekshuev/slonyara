#pragma once

#include "connectionprofile.h"
#include <QThread>
#include "connectionstopstate.h"

class ConnectionWorker final : public QThread
{
    Q_OBJECT
public:
    ConnectionWorker(ConnectionProfile profile, QString password);
    ~ConnectionWorker() override;
    void stop();
signals:
    void opened();
    void failed(const QString &details);

private:
    void run() override;
    ConnectionProfile m_profile;
    QString m_password;
    ConnectionStopState m_stop;
};
