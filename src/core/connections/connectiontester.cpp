#include "connectiontester.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>

ConnectionTestResult ConnectionTester::test(const ConnectionProfile &profile, const QString &password)
{
    const QString connectionName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ConnectionTestResult res;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connectionName);
        db.setDatabaseName(profile.database);
        db.setHostName(profile.host);
        db.setUserName(profile.user);
        db.setPassword(password);
        db.setPort(profile.port);
        db.setConnectOptions(QStringLiteral("connect_timeout=5"));

        if (db.open()) {
            res.success = true;
        } else {
            res.error = db.lastError().text();
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return res;
}
