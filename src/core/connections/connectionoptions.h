#pragma once

#include "connectionprofile.h"
#include <string>

namespace ConnectionOptions {
inline std::string build(const ConnectionProfile &profile, const QString &password)
{
    const auto quote = [](QString value) {
        value.replace("\\", "\\\\");
        value.replace("'", "\\'");
        return "'" + value.toStdString() + "'";
    };
    return "host=" + quote(profile.host)
           + " port=" + std::to_string(profile.port)
           + " user=" + quote(profile.user)
           + " dbname=" + quote(profile.database)
           + " password=" + quote(password)
           + " connect_timeout=10 application_name=Slonyara";
}
} // namespace ConnectionOptions
