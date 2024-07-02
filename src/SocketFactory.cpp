#include "SocketFactory.hpp"
#include "SocketDccp.hpp"
#include "SocketSctp.hpp"
#include "SocketUdp.hpp"
#include "SocketUnix.hpp"
#include "SocketUnixForked.hpp"

using namespace std;

unique_ptr<Socket> createSocket(const NetworkConfiguration& config, TaskScheduler& ts)
{
    switch (config.protocol)
    {
        case NetworkProtocol::TCP: return make_unique<SocketTcp>(config.locals, ts);
        case NetworkProtocol::UDP: return make_unique<SocketUdp>(config.locals, ts);
        case NetworkProtocol::UDP_Lite: return make_unique<SocketUdp>(config.locals, ts, IPPROTO_UDPLITE);
        case NetworkProtocol::DCCP: return make_unique<SocketDccp>(config.locals, ts);
        case NetworkProtocol::Unix: return make_unique<SocketUnix>(ts);
        case NetworkProtocol::UnixForked: return make_unique<SocketUnixForked>(ts);
        default: return make_unique<SocketSctp>(config, ts);
    }
}
