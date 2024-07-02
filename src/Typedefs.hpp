#pragma once

#include <chrono>
#include "IPSocket.hpp"

using ChatMessage = std::string;
using Delay = std::chrono::milliseconds;
using AssocId = sctp_assoc_t;
using FD = int;
using FDs = std::vector<FD>;
using SocketParam = int;

using LocalIPSocket = IPSocket;
using RemoteIPSocket = IPSocket;
using LocalIPSockets = IPSockets;
using RemoteIPSockets = IPSockets;
