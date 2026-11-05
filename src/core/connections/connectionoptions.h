#pragma once

#include "connectionprofile.h"
#include "connectionencoding.h"
#include <string>

namespace ConnectionOptions {
inline std::string build(const ConnectionProfile &profile, const QString &password)
{
    return ConnectionEncoding::build(profile.host.toStdString(), profile.port,
            profile.user.toStdString(), profile.database.toStdString(), password.toStdString());
}
} // namespace ConnectionOptions
