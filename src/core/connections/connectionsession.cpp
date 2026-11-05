#include "connectionsession.h"

#include <QElapsedTimer>
#include <QMutexLocker>
#include <pqxx/pqxx>
#include <stdexcept>

ConnectionSession::ConnectionSession(ConnectionProfile profile, QString password)
    : m_profile(std::move(profile)), m_password(std::move(password))
{
}

ConnectionSession::~ConnectionSession()
{
    stop();
    wait();
}

void ConnectionSession::stop()
{
    QMutexLocker lock(&m_mutex);
    requestInterruption();
    m_stopped.wakeAll();
}

void ConnectionSession::run()
{
    try {
        auto options = "host=" + m_profile.host.toStdString() + " port="
                       + std::to_string(m_profile.port) + " user=" + m_profile.user.toStdString()
                       + " dbname=" + m_profile.database.toStdString() + " password="
                       + m_password.toStdString() + " connect_timeout=10 application_name=Slonyara";
        m_password.clear();
        QElapsedTimer timer;
        timer.start();
        pqxx::connecting pending{options};
        options.clear();
        while (!pending.done()) {
            if (isInterruptionRequested())
                return;
            if (timer.elapsed() >= 10000)
                throw std::runtime_error("Connection timed out.");
            pending.process();
            if (!pending.done())
                QThread::msleep(20);
        }
        auto connection = std::move(pending).produce();
        QMutexLocker lock(&m_mutex);
        if (isInterruptionRequested())
            return;
        emit opened();
        while (!isInterruptionRequested())
            m_stopped.wait(&m_mutex);
    } catch (const std::exception &error) {
        m_password.clear();
        if (!isInterruptionRequested())
            emit failed(QString::fromUtf8(error.what()));
    }
}
