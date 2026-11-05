#pragma once

#include "connectionsession.h"
#include <memory>

class ConnectionWorker;

class PostgresSession final : public ConnectionSession
{
public:
    PostgresSession(ConnectionProfile profile, QString password);
    ~PostgresSession() override;
    void start() override;
    void stop() override;

private:
    std::unique_ptr<ConnectionWorker> m_worker;
};
