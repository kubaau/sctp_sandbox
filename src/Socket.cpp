#include "Socket.hpp"
#include <tools/AsyncTask.hpp>
#include <tools/ContainerOperators.hpp>
#include <tools/Contains.hpp>
#include <tools/TaskScheduler.hpp>
#include "SocketConfiguration.hpp"
#include "SocketErrorChecks.hpp"
#include "SocketIO.hpp"

using namespace std;

static auto chooseSocketFamily(const LocalIPSockets& locals)
{
    return any_of(locals, [](const auto& ip) { return ip.isIPv6(); }) ? AF_INET6 : AF_INET;
}

Socket::Socket(const LocalIPSockets& locals,
               TaskScheduler& ts,
               Type type,
               Protocol protocol,
               Family fam,
               DeferCreation defer_creation)
    : family(fam == AF_UNSPEC ? chooseSocketFamily(locals) : fam), type(type), task_scheduler(ts)
{
    if (not defer_creation)
    {
        fd = FileDescriptor{checkedSocket(socket(family, type, protocol))};
        DEBUG_LOG << "Created socket: fd = " << fd << ", family = " << toString(family);
    }
}

void Socket::listen(BacklogCount backlog_count)
{
    if (shouldListen())
    {
        checkListen(::listen(fd, backlog_count));
        DEBUG_LOG << "Listening: fd = " << fd << ", backlog_count = " << backlog_count;
    }
}

void Socket::connect(const RemoteIPSockets&) {}

void Socket::connectMultihomed(const RemoteIPSockets&)
{
    WARN_LOG << "Impossible on this socket type";
}

void Socket::receive()
{
    using namespace RangeOperators;
    AsyncTasks tasks;
    for (auto fd : selectFds())
    {
        DEBUG_LOG << "Receiving message on fd = " << fd;
        tasks += asyncTask(&Socket::handleMessage, this, fd);
    }
    join(tasks);
}

void Socket::configure(FD fd)
{
    configureReuseAddr(fd);
    configureNonBlockingMode(fd);
    if (family != AF_UNIX)
    {
        configureDscp(fd, 63, family);
        configureTTL(fd, 255);
    }
}

void Socket::bind(FD fd, const LocalIPSockets& locals)
{
    const auto& local = locals.front();
    const sockaddr_storage saddr = local;
    INFO_LOG << "Binding socket: fd = " << fd << ", addr = " << local;
    checkBind(::bind(fd, asSockaddrPtr(saddr), local.sizeofSockaddr()));
    DEBUG_LOG << "Bound addresses: " << getBoundAddresses(fd);
}

template <class F>
IPSockets getAddresses(FD fd, F func)
{
    sockaddr_storage saddr_storage{};
    socklen_t saddr_len = sizeof(sockaddr_storage);
    checkGetsockname(func(fd, asSockaddrPtr(saddr_storage), &saddr_len));
    return {saddr_storage};
}

LocalIPSockets Socket::getBoundAddresses(FD fd) const
{
    return getAddresses(fd, getsockname);
}

RemoteIPSockets Socket::getPeerAddresses(FD fd) const
{
    return getAddresses(fd, getpeername);
}

FDs Socket::selectFds()
{
    fd_set.reset();
    fd_set.set(fd);
    return fd_set.select();
}

IOBuffer& Socket::getBuffer(FD)
{
    return *main_msg_buffer;
}

bool Socket::shouldListen() const
{
    return type != SOCK_DGRAM;
}

void Socket::connect(FD fd, const RemoteIPSocket& remote)
{
    INFO_LOG << "Connecting to " << remote << " from fd = " << fd;
    const sockaddr_storage saddr = remote;
    checkConnect(::connect(fd, asSockaddrPtr(saddr), remote.sizeofSockaddr()));
}

pair<FileDescriptor, RemoteIPSocket> Socket::accept()
{
    DEBUG_LOG << "Accepting: fd = " << fd;
    sockaddr_storage saddr_storage{};
    socklen_t saddr_len = sizeof(sockaddr_storage);
    FileDescriptor accept_result{
        checkedAccept(accept4(fd, asSockaddrPtr(saddr_storage), &saddr_len, SOCK_NONBLOCK | SOCK_CLOEXEC))};
    const auto remote = RemoteIPSocket{saddr_storage};
    DEBUG_LOG << "Accepted remote fd = " << accept_result << ", peer address = " << remote;
    return {move(accept_result), remote};
}

void Socket::scheduleReestablishment(const RemoteIPSocket& remote)
{
    DEBUG_LOG << "Scheduled delayed reestablishment to remote " << remote;
    task_scheduler.schedule([=, this] { connect({remote}); }, 5s);
}
