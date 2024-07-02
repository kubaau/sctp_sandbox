#pragma once

#include <tools/Typedefs.hpp>
#include <tuple>
#include "SocketApi.hpp"

using IP = std::string;
using Port = u16;
using Family = sa_family_t;

struct IPSocket
{
    IPSocket(IP, Port = any_port);
    IPSocket(const sockaddr&);
    IPSocket(const sockaddr_storage&);

    operator sockaddr_storage() const;

    IP addr;
    Port port;
    Family family;
    auto tieAll() const { return std::tie(addr, port, family); }

    bool isIPv6() const;
    Size sizeofSockaddr() const;

private:
    static constexpr auto any_port = 0;

    operator sockaddr_in() const;
    operator sockaddr_in6() const;
};
using IPSockets = std::vector<IPSocket>;

std::ostream& operator<<(std::ostream&, const IPSocket&);

using Sockaddrs = std::vector<Byte>;
Sockaddrs toSockaddrs(const IPSockets&);
