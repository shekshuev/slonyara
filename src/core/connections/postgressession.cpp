#include "postgressession.h"
#include "connectionworker.h"

PostgresSession::PostgresSession(ConnectionProfile profile, QString password)
    : ConnectionSession(profile),
      m_worker(std::make_unique<ConnectionWorker>(std::move(profile), std::move(password)))
{
    connect(m_worker.get(), &ConnectionWorker::opened, this, &ConnectionSession::opened);
    connect(m_worker.get(), &ConnectionWorker::failed, this, &ConnectionSession::failed);
    connect(m_worker.get(), &QThread::finished, this, &ConnectionSession::finished);
}

PostgresSession::~PostgresSession() = default;

void PostgresSession::start()
{
    m_worker->start();
}
void PostgresSession::stop()
{
    m_worker->stop();
}
