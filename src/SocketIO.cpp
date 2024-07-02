#include "SocketIO.hpp"

using namespace std;

string toString(Family family)
{
    switch (family)
    {
        case AF_INET: return "AF_INET";
        case AF_INET6: return "AF_INET6";
        case AF_UNIX: return "AF_UNIX";
        case AF_UNSPEC: return "AF_UNSPEC";
        default: return to_string(family);
    }
}

string toString(sctp_spinfo_state state)
{
    switch (state)
    {
        case SCTP_INACTIVE: return "SCTP_INACTIVE";
        case SCTP_PF: return "SCTP_PF";
        case SCTP_ACTIVE: return "SCTP_ACTIVE";
        case SCTP_UNCONFIRMED: return "SCTP_UNCONFIRMED";
        case SCTP_UNKNOWN: return "SCTP_UNKNOWN";
        default: return to_string(state);
    }
}
