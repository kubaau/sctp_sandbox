#pragma once

#include "Typedefs.hpp"

class FDSet
{
public:
    using Timeout = std::chrono::microseconds;

    FDSet();

    void reset();
    void set(FD);
    FDs select(Timeout = std::chrono::milliseconds{100});

private:
    int selectImpl(Timeout);

    fd_set fds;
    FD fd_max;
    FDs selected_fds;
};
