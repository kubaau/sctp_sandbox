#include "IPSocket.hpp"
#include <tools/ExtendedRangeStlAlgorithms.hpp>
#include "SocketErrorChecks.hpp"

using namespace std;

namespace
{
    auto isFamily(IP addr, Family family)
    {
        sockaddr_storage ignore_pton_result;
        return succeededInetpton(inet_pton(family, addr.c_str(), &ignore_pton_result));
    }

    Family detectFamily(IP addr)
    {
        return isFamily(addr, AF_INET) ? AF_INET : isFamily(addr, AF_INET6) ? AF_INET6 : AF_UNIX;
    }
}

IPSocket::IPSocket(IP addr, Port port) : addr(addr), port(port), family(detectFamily(addr)) {}

IPSocket::IPSocket(const sockaddr& saddr) : IPSocket(reinterpret_cast<const sockaddr_storage&>(saddr)) {}

IPSocket::IPSocket(const sockaddr_storage& saddr_storage)
{
    const void* sin_addr;

#define FILL_FROM_SADDR_STORAGE(Suffix)                                                     \
    {                                                                                       \
        const auto& saddr_in = reinterpret_cast<const sockaddr_in##Suffix&>(saddr_storage); \
        sin_addr = &saddr_in.sin##Suffix##_addr;                                            \
        port = ntohs(saddr_in.sin##Suffix##_port);                                          \
    }

    family = saddr_storage.ss_family;
    if (isIPv6())
        FILL_FROM_SADDR_STORAGE(6)
    else
        FILL_FROM_SADDR_STORAGE()

    addr.resize(isIPv6() ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN);
    checkInetntop(inet_ntop(family, sin_addr, addr.data(), addr.size()));
}

IPSocket::operator sockaddr_storage() const
{
#define RETURN_SOCKADDR_STORAGE(Suffix)                                          \
    {                                                                            \
        const sockaddr_in##Suffix saddr = *this;                                 \
        sockaddr_storage saddr_storage = {};                                     \
        return saddr_storage = reinterpret_cast<const sockaddr_storage&>(saddr); \
    }

    if (isIPv6())
        RETURN_SOCKADDR_STORAGE(6)
    else
        RETURN_SOCKADDR_STORAGE()
}

bool IPSocket::isIPv6() const
{
    return family == AF_INET6;
}

Size IPSocket::sizeofSockaddr() const
{
    return isIPv6() ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
}

#define RETURN_SOCKADDR_IN(Suffix)                                                            \
    sockaddr_in##Suffix ret = {};                                                             \
    ret.sin##Suffix##_family = family;                                                        \
    ret.sin##Suffix##_port = htons(port);                                                     \
    checkInetpton(inet_pton(family, addr.c_str(), &ret.sin##Suffix##_addr.s##Suffix##_addr)); \
    return ret;

IPSocket::operator sockaddr_in6() const
{
    RETURN_SOCKADDR_IN(6);
}

IPSocket::operator sockaddr_in() const
{
    RETURN_SOCKADDR_IN();
}

ostream& operator<<(ostream& os, const IPSocket& ip_socket)
{
    return os << ip_socket.addr << ":" << ip_socket.port;
}

Sockaddrs toSockaddrs(const IPSockets& ip_sockets)
{
    Sockaddrs ret;
    for (const auto& ip_socket : ip_sockets)
    {
        const sockaddr_storage saddr_storage = ip_socket;
        copy_n(reinterpret_cast<const Byte*>(&saddr_storage), ip_socket.sizeofSockaddr(), back_inserter(ret));
    }
    return ret;
}
