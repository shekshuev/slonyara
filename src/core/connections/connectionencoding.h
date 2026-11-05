#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace ConnectionEncoding {
inline std::string quote(std::string_view value)
{
    std::string encoded = "'";
    for (char character : value) {
        if (character == '\\' || character == '\'')
            encoded += '\\';
        encoded += character;
    }
    encoded += '\'';
    return encoded;
}

inline std::string build(std::string_view host, std::uint16_t port, std::string_view user,
        std::string_view database, std::string_view password)
{
    return "host=" + quote(host) + " port=" + std::to_string(port) + " user=" + quote(user)
            + " dbname=" + quote(database) + " password=" + quote(password)
            + " connect_timeout=10 application_name=Slonyara";
}
} // namespace ConnectionEncoding
