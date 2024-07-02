#pragma once

#include "NetworkConfiguration.hpp"

using Lifetime = std::chrono::milliseconds;
struct NetworkTaskParams
{
    NetworkConfiguration config;
    Name name = "network";
    Delay delay{};
    Lifetime lifetime{};
};
void networkTask(const NetworkTaskParams&);
