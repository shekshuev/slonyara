#pragma once

#include "connectionprofile.h"
#include <QObject>
#include <utility>

class ConnectionSession : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionSession(ConnectionProfile profile) : m_profile(std::move(profile)) {}
    virtual void start() = 0;
    virtual void stop() = 0;
    const ConnectionProfile &profile() const { return m_profile; }

signals:
    void opened();
    void failed(const QString &details);
    void finished();

private:
    ConnectionProfile m_profile;
};
