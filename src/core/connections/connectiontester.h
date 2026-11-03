#pragma once

#include "connectionprofile.h"

struct ConnectionTestResult
{
    bool success = false;
    QString error;
};

class ConnectionTester
{
public:
    static ConnectionTestResult test(const ConnectionProfile &profile, const QString &password);
};
