#pragma once

#include "connectionprofile.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

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
    QMutex m_mutex;
    QWaitCondition m_stopped;
};

