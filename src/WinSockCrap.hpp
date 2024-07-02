#pragma once

#include <ws2sctp.h>

using sa_family_t = ADDRESS_FAMILY;

int close(int)
{
    return 0;
}

constexpr auto SOL_SCTP = 0;

constexpr auto F_GETFL = 0;
constexpr auto F_SETFL = 0;
constexpr auto O_NONBLOCK = 0;
int fcntl(int, int, int)
{
    return 0;
}
