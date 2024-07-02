#include "FDSet.hpp"
#include <tools/Repeat.hpp>
#include "SocketErrorChecks.hpp"
#include "ToTimeval.hpp"

using namespace std::chrono;

FDSet::FDSet()
{
    selected_fds.reserve(FD_SETSIZE);
    reset();
}

void FDSet::reset()
{
    FD_ZERO(&fds);
    fd_max = 1;
}

void FDSet::set(FD fd)
{
    FD_SET(fd, &fds);
    if (fd >= fd_max)
        fd_max = fd + 1;
}

FDs FDSet::select(Timeout timeout)
{
    selected_fds.clear();
    if (selectImpl(timeout))
        repeat(fd_max) if (FD_ISSET(i, &fds)) selected_fds.push_back(i);
    return selected_fds;
}

int FDSet::selectImpl(Timeout timeout)
{
    constexpr auto write_fds = nullptr;
    constexpr auto except_fds = nullptr;
    auto timeout_timeval = toTimeval(timeout);
    return checkedSelect(::select(fd_max, &fds, write_fds, except_fds, &timeout_timeval));
}
