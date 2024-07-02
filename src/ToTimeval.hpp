#pragma once

#include <tools/CountTime.hpp>
#include "SocketApi.hpp"

template <class Time>
constexpr auto toTimeval(Time time)
{
    using namespace std::chrono;
    using TimevalRep = decltype(timeval::tv_sec);
    return timeval{count<seconds, TimevalRep>(time),
                   count<microseconds, TimevalRep>(time - duration_cast<seconds>(time))};
}
