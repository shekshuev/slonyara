#include "core/connections/connectiontester.h"

#include <QTcpServer>
#include <QTest>
#include <QtConcurrent/QtConcurrentRun>
#include <pqxx/pqxx>
#include <map>
#include <memory>

class ConnectionTesterTest final : public QObject
{
    Q_OBJECT
private:
    std::unique_ptr<pqxx::connection> m_admin;
    ConnectionProfile m_profile;
    const QString m_password = QStringLiteral("password with ' quotes \\ and spaces");
    bool m_roleCreated = false;
    bool m_databaseCreated = false;

    int activeConnections() {
        pqxx::nontransaction query(*m_admin);
        return query.exec("SELECT count(*) FROM pg_stat_activity WHERE usename = "
            + query.quote(m_profile.user.toStdString()) + " AND datname = "
            + query.quote(m_profile.database.toStdString())).one_row()[0].as<int>();
    }
private slots:
    void initTestCase() {
        QVERIFY2(qEnvironmentVariableIsSet("SLONYARA_TEST_PG_HOST"), "See tests/README.md for PostgreSQL configuration");
        m_profile.host = qEnvironmentVariable("SLONYARA_TEST_PG_HOST");
        bool valid = false;
        const int port = qEnvironmentVariable("SLONYARA_TEST_PG_PORT", "5432").toInt(&valid);
        QVERIFY(valid && port > 0 && port <= 65535);
        m_profile.port = quint16(port);
        const auto suffix = QUuid::createUuid().toString(QUuid::Id128).left(12);
        m_profile.user = "tester role ' \\ " + suffix;
        m_profile.database = "tester database ' \\ " + suffix;
        const std::map<std::string, std::string> params{
            {"host", m_profile.host.toStdString()}, {"port", std::to_string(port)},
            {"user", qEnvironmentVariable("SLONYARA_TEST_PG_USER", "postgres").toStdString()},
            {"password", qEnvironmentVariable("SLONYARA_TEST_PG_PASSWORD").toStdString()},
            {"dbname", qEnvironmentVariable("SLONYARA_TEST_PG_DATABASE", "postgres").toStdString()},
            {"connect_timeout", "5"}};
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
    void cleanupTestCase() {
        if (!m_admin || !m_admin->is_open()) return;
        try {
            pqxx::nontransaction cleanup(*m_admin);
            if (m_databaseCreated) cleanup.exec("DROP DATABASE " + cleanup.quote_name(m_profile.database.toStdString()));
            if (m_roleCreated) cleanup.exec("DROP ROLE " + cleanup.quote_name(m_profile.user.toStdString()));
        } catch (const std::exception &error) {
            QFAIL(error.what());
        }
    }
    void connectsWithSpecialCharactersAndClosesConnection() {
        const auto result = ConnectionTester::test(m_profile, m_password);
        QVERIFY2(result.success, qPrintable(result.error));
        QVERIFY(result.error.isEmpty());
        QTRY_COMPARE(activeConnections(), 0);
        const auto repeated = ConnectionTester::test(m_profile, m_password);
        QVERIFY2(repeated.success, qPrintable(repeated.error));
        QTRY_COMPARE(activeConnections(), 0);
    }
    void invalidCredentials_data() {
        QTest::addColumn<bool>("wrongPassword");
        QTest::newRow("wrong-password") << true;
        QTest::newRow("missing-database") << false;
    }
    void invalidCredentials() {
        QFETCH(bool, wrongPassword);
        auto profile = m_profile;
        if (!wrongPassword) profile.database += " missing";
        const auto result = ConnectionTester::test(profile, wrongPassword ? "wrong password" : m_password);
        QVERIFY(!result.success);
        QVERIFY(!result.error.isEmpty());
        QTRY_COMPARE(activeConnections(), 0);
    }
    void connectionRefusalReturnsError() {
        QTcpServer unused;
        QVERIFY(unused.listen(QHostAddress::LocalHost));
        auto profile = m_profile;
        profile.host = "127.0.0.1";
        profile.port = unused.serverPort();
        unused.close();
        const auto result = ConnectionTester::test(profile, m_password);
        QVERIFY(!result.success);
        QVERIFY(!result.error.isEmpty());
    }
    void unresponsiveServerTimesOut() {
        QTcpServer hanging;
        QVERIFY(hanging.listen(QHostAddress::LocalHost));
        auto profile = m_profile;
        profile.host = "127.0.0.1";
        profile.port = hanging.serverPort();
        auto future = QtConcurrent::run([profile, password = m_password] {
            return ConnectionTester::test(profile, password);
        });
        QTRY_VERIFY(hanging.hasPendingConnections());
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 15000);
        const auto result = future.result();
        QVERIFY(!result.success);
        QVERIFY(!result.error.isEmpty());
    }
};

QTEST_GUILESS_MAIN(ConnectionTesterTest)
#include "connectiontestertest.moc"
