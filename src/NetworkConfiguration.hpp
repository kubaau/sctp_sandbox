#pragma once

#include <tools/Args.hpp>
#include "NetworkProtocol.hpp"
#include "Typedefs.hpp"

struct NetworkConfiguration
{
    NetworkConfiguration(const Args&);

    LocalIPSockets locals;
    RemoteIPSockets remotes;
    NetworkProtocol protocol;
    SocketParam path_mtu{};
    SocketParam max_seg{};
    Size big_msg_size{};
};
