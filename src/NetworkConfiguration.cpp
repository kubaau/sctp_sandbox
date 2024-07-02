#include "NetworkConfiguration.hpp"
#include <tools/ExtendedRangeStlAlgorithms.hpp>
#include <tools/ThreadSafeLogger.hpp>
#include <tools/ToLower.hpp>

using namespace std;

static auto getProtocol(Arg arg)
{
    map<Arg, NetworkProtocol> protocol = {{"sctp", NetworkProtocol::SCTP},
                                          {"tcp", NetworkProtocol::TCP},
                                          {"udp", NetworkProtocol::UDP},
                                          {"udp_lite", NetworkProtocol::UDP_Lite},
                                          {"dccp", NetworkProtocol::DCCP},
                                          {"unix", NetworkProtocol::Unix},
                                          {"fork", NetworkProtocol::UnixForked}};
    return protocol[toLower(arg)];
}

NetworkConfiguration::NetworkConfiguration(const Args& args)
{
    IPSockets* filling = &locals;
    for (auto i = 1u; i < args.size() - 1; ++i)
    {
        const auto arg = args[i];

        if (arg.front() == '-')
        {
            if (arg == "-p")
                protocol = getProtocol(args[++i]);
            else if (arg == "-mtu")
                path_mtu = stoi(args[++i]);
            else if (arg == "-seg")
                max_seg = stoi(args[++i]);
            else if (arg == "-msg")
                big_msg_size = stoi(args[++i]);
            else if (arg == "-r")
                filling = &remotes;
        }
        else
            filling->push_back(IPSocket{arg, static_cast<Port>(stoi(args[++i]))});
    }
}
