#include "connectionworker.h"
#include "connectionoptions.h"

#include <QElapsedTimer>
#include <pqxx/pqxx>
#include <stdexcept>
#include <utility>

ConnectionWorker::ConnectionWorker(ConnectionProfile profile, QString password)
    : m_profile(std::move(profile)), m_password(std::move(password))
{
}

ConnectionWorker::~ConnectionWorker()
{
    stop();
    wait();
}

void ConnectionWorker::stop()
{
    m_stop.requestStop();
}

void ConnectionWorker::run()
{
    try {
        auto options = ConnectionOptions::build(m_profile, m_password);
        m_password.clear();
        QElapsedTimer timer;
        timer.start();
        pqxx::connecting pending{ options };
        options.clear();
        while (!pending.done()) {
            if (m_stop.stopRequested())
                return;
            if (timer.elapsed() >= 10000)
                throw std::runtime_error("Connection timed out.");
            pending.process();
            if (!pending.done())
                QThread::msleep(20);
        }
        auto connection = std::move(pending).produce();
        m_stop.waitUntilStopped([this] { emit opened(); });
    } catch (const std::exception &error) {
        m_password.clear();
        if (!m_stop.stopRequested())
            emit failed(QString::fromUtf8(error.what()));
    }
}
