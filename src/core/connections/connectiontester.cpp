#include "connectiontester.h"
#include "connectionoptions.h"
#include <pqxx/pqxx>
#include <exception>

ConnectionTestResult ConnectionTester::test(const ConnectionProfile &profile, const QString &password)
{
    ConnectionTestResult res;

    try {
        pqxx::connection connection{ConnectionOptions::build(profile, password)};
        res.success = connection.is_open();
    } catch (const std::exception &error) {
        res.error = QString::fromUtf8(error.what());
    }
    return res;
}
