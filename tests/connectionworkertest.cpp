#include "core/connections/connectionworker.h"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <pqxx/pqxx>
#include <map>
#include <memory>

class ConnectionWorkerTest final : public QObject
{
    Q_OBJECT
private:
    std::unique_ptr<pqxx::connection> m_admin;
    ConnectionProfile m_profile;
    QString m_password = QStringLiteral("test password ' \\ end");
    bool m_roleCreated = false;
    bool m_databaseCreated = false;

    int activeConnections()
    {
        pqxx::nontransaction query(*m_admin);
        return query
                .exec("SELECT count(*) FROM pg_stat_activity WHERE usename = "
                        + query.quote(m_profile.user.toStdString())
                        + " AND datname = " + query.quote(m_profile.database.toStdString()))
                .one_row()[0]
                .as<int>();
    }
    static ConnectionProfile localProfile(quint16 port)
    {
        ConnectionProfile profile;
        profile.host = "127.0.0.1";
        profile.port = port;
        profile.user = "test";
        profile.database = "test";
        return profile;
    }

private slots:
    void initTestCase()
    {
        QVERIFY2(qEnvironmentVariableIsSet("SLONYARA_TEST_PG_HOST"),
                "Set SLONYARA_TEST_PG_HOST and the other SLONYARA_TEST_PG_* variables; see "
                "tests/README.md");
        m_profile.host = qEnvironmentVariable("SLONYARA_TEST_PG_HOST");
        bool validPort = false;
        const int port = qEnvironmentVariable("SLONYARA_TEST_PG_PORT", "5432").toInt(&validPort);
        QVERIFY(validPort && port > 0 && port <= 65535);
        m_profile.port = quint16(port);
        const auto suffix = QUuid::createUuid().toString(QUuid::Id128).left(12);
        m_profile.user = "worker role ' \\ " + suffix;
        m_profile.database = "worker database ' \\ " + suffix;
        const std::map<std::string, std::string> params{ { "host", m_profile.host.toStdString() },
            { "port", std::to_string(port) },
            { "user", qEnvironmentVariable("SLONYARA_TEST_PG_USER", "postgres").toStdString() },
            { "password", qEnvironmentVariable("SLONYARA_TEST_PG_PASSWORD").toStdString() },
            { "dbname",
                    qEnvironmentVariable("SLONYARA_TEST_PG_DATABASE", "postgres").toStdString() },
            { "connect_timeout", "5" } };
        try {
            m_admin = std::make_unique<pqxx::connection>(params);
            pqxx::nontransaction setup(*m_admin);
            setup.exec("CREATE ROLE " + setup.quote_name(m_profile.user.toStdString())
                    + " LOGIN PASSWORD " + setup.quote(m_password.toStdString()));
            m_roleCreated = true;
            setup.exec("CREATE DATABASE " + setup.quote_name(m_profile.database.toStdString())
                    + " OWNER " + setup.quote_name(m_profile.user.toStdString()));
            m_databaseCreated = true;
        } catch (const std::exception &error) {
            QFAIL(error.what());
        }
    }
    void cleanupTestCase()
    {
        if (!m_admin || !m_admin->is_open())
            return;
        try {
            pqxx::nontransaction cleanup(*m_admin);
            if (m_databaseCreated)
                cleanup.exec(
                        "DROP DATABASE " + cleanup.quote_name(m_profile.database.toStdString()));
            if (m_roleCreated)
                cleanup.exec("DROP ROLE " + cleanup.quote_name(m_profile.user.toStdString()));
        } catch (const std::exception &error) {
            QFAIL(error.what());
        }
    }
    void opensWithSpecialCharactersAndReleasesConnection()
    {
        ConnectionWorker worker(m_profile, m_password);
        QSignalSpy opened(&worker, &ConnectionWorker::opened);
        QSignalSpy failed(&worker, &ConnectionWorker::failed);
        worker.start();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 12000);
        QCOMPARE(failed.count(), 0);
        QVERIFY(worker.isRunning());
        QTRY_COMPARE(activeConnections(), 1);
        worker.stop();
        QVERIFY(worker.wait(3000));
        QTRY_COMPARE(activeConnections(), 0);
        QCOMPARE(opened.count(), 1);
        QCOMPARE(failed.count(), 0);
    }
    void invalidCredentials_data()
    {
        QTest::addColumn<bool>("wrongPassword");
        QTest::newRow("wrong-password") << true;
        QTest::newRow("missing-database") << false;
    }
    void invalidCredentials()
    {
        QFETCH(bool, wrongPassword);
        auto profile = m_profile;
        if (!wrongPassword)
            profile.database += " missing";
        ConnectionWorker worker(profile, wrongPassword ? "incorrect password" : m_password);
        QSignalSpy opened(&worker, &ConnectionWorker::opened);
        QSignalSpy failed(&worker, &ConnectionWorker::failed);
        worker.start();
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 12000);
        QVERIFY(worker.wait(3000));
        QCOMPARE(opened.count(), 0);
        QVERIFY(!failed.first().first().toString().isEmpty());
        QTRY_COMPARE(activeConnections(), 0);
    }
    void refusedConnectionReportsFailure()
    {
        QTcpServer unused;
        QVERIFY(unused.listen(QHostAddress::LocalHost));
        const auto port = unused.serverPort();
        unused.close();
        ConnectionWorker worker(localProfile(port), "test");
        QSignalSpy opened(&worker, &ConnectionWorker::opened);
        QSignalSpy failed(&worker, &ConnectionWorker::failed);
        worker.start();
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 12000);
        QVERIFY(worker.wait(3000));
        QCOMPARE(opened.count(), 0);
    }
    void cancellationDuringHandshakeIsSilent()
    {
        QTcpServer hanging;
        QVERIFY(hanging.listen(QHostAddress::LocalHost));
        ConnectionWorker worker(localProfile(hanging.serverPort()), "test");
        QSignalSpy opened(&worker, &ConnectionWorker::opened);
        QSignalSpy failed(&worker, &ConnectionWorker::failed);
        worker.start();
        QTRY_VERIFY(hanging.hasPendingConnections());
        auto *socket = hanging.nextPendingConnection();
        worker.stop();
        QVERIFY(worker.wait(3000));
        QCoreApplication::processEvents();
        QCOMPARE(opened.count(), 0);
        QCOMPARE(failed.count(), 0);
        QTRY_COMPARE(socket->state(), QAbstractSocket::UnconnectedState);
    }
    void handshakeTimesOut()
    {
        QTcpServer hanging;
        QVERIFY(hanging.listen(QHostAddress::LocalHost));
        ConnectionWorker worker(localProfile(hanging.serverPort()), "test");
        QSignalSpy opened(&worker, &ConnectionWorker::opened);
        QSignalSpy failed(&worker, &ConnectionWorker::failed);
        worker.start();
        QTRY_VERIFY(hanging.hasPendingConnections());
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 13000);
        QVERIFY(worker.wait(3000));
        QCOMPARE(opened.count(), 0);
        QVERIFY(failed.first().first().toString().contains("timed out"));
    }
    void destructionStopsAnOpenConnection()
    {
        auto worker = std::make_unique<ConnectionWorker>(m_profile, m_password);
        QSignalSpy opened(worker.get(), &ConnectionWorker::opened);
        worker->start();
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 12000);
        QTRY_COMPARE(activeConnections(), 1);
        worker.reset();
        QTRY_COMPARE(activeConnections(), 0);
    }
};

QTEST_GUILESS_MAIN(ConnectionWorkerTest)
#include "connectionworkertest.moc"
