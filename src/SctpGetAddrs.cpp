#include "SctpGetAddrs.hpp"
#include <tools/ContainerOperators.hpp>
#include <tools/PtrMagic.hpp>
#include <tools/Repeat.hpp>
#include "SocketErrorChecks.hpp"

using namespace std;
using namespace RangeOperators;

namespace
{
    enum class AddrType
    {
        LOCAL,
        PEER
    };

    auto getAddrs(FD fd, AssocId assoc_id, AddrType type)
    {
        IPSockets ret;

        sockaddr* saddrs;
        auto count = checkedGetaddrs(type == AddrType::LOCAL ? sctp_getladdrs(fd, assoc_id, &saddrs) :
                                                               sctp_getpaddrs(fd, assoc_id, &saddrs));
        auto current_saddr = saddrs;
        repeat(count)
        {
            ret += *current_saddr;
            moveNBytes(current_saddr, ret.back().sizeofSockaddr());
            // we're iterating through an array of sockaddr's, but this array actually contains sockaddr_in's
            // and sockaddr_in6's - to get the next element of the array, we need to push the pointer forwards n bytes,
            // where n is the size of either sockaddr_in or sockaddr_in6 (depending on the current element)
        }
        if (count) // addrs is undefined if get returns 0 or less
            type == AddrType::LOCAL ? sctp_freeladdrs(saddrs) : sctp_freepaddrs(saddrs);

        return ret;
    }
} // namespace

IPSockets getLaddrs(FD fd, AssocId assoc_id)
{
    return getAddrs(fd, assoc_id, AddrType::LOCAL);
}

IPSockets getPaddrs(FD fd, AssocId assoc_id)
{
    return getAddrs(fd, assoc_id, AddrType::PEER);
}
