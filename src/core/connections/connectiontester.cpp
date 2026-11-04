#include "connectiontester.h"
#include <pqxx/pqxx>
#include <exception>
// namespace

ConnectionTestResult ConnectionTester::test(const ConnectionProfile &profile, const QString &password)
{
    ConnectionTestResult res;
    std::string conn_opts = "host=" + profile.host.toStdString() + " port="
                            + std::to_string(profile.port) + " user=" + profile.user.toStdString()
                            + " password=" + password.toStdString()
                            + " dbname=" + profile.database.toStdString();

    try {
        pqxx::connection connection{conn_opts};
        res.success = connection.is_open();
    } catch (const std::exception &error) {
        res.error = QString::fromUtf8(error.what());
    }
    return res;
}
