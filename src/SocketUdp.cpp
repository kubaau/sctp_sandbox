#include "SocketUdp.hpp"
#include <tools/ComparisonOperators.hpp>
#include "Constants.hpp"
#include "SocketErrorChecks.hpp"

using namespace std;

SocketUdp::SocketUdp(const LocalIPSockets& locals, TaskScheduler& ts, Protocol protocol)
    : Socket(locals, ts, SOCK_DGRAM, protocol), local(locals.front().addr)
{
    SocketUdp::configure(fd);
    DEBUG_LOG << "Configured fd = " << fd;
    SocketUdp::bind(fd, locals);
}

SocketUdp::~SocketUdp()
{
    SocketUdp::send(quit_msg);
}

void SocketUdp::connect(const RemoteIPSockets& remotes)
{
    for (const auto& remote : remotes)
        handleCommUp(remote);
}

void SocketUdp::send(const ChatMessage& msg)
{
    for (const auto& p : peers)
        send(msg, p.first);
}

namespace
{
    constexpr auto keep_alive_period = 1s;
    constexpr auto keep_alive_probes = 3;
    constexpr auto keep_alive_msg = "ka";
    constexpr auto keep_alive_ack_msg = "kack";
} // namespace

void SocketUdp::handleMessage(FD fd)
{
    sockaddr_storage from_storage{};
    socklen_t from_len = sizeof(from_storage);

    auto& msg_buffer = getBuffer(fd);

    msg_buffer[checkedReceive(
        recvfrom(fd, msg_buffer.data(), msg_buffer.size(), ignore_flags, asSockaddrPtr(from_storage), &from_len))] = 0;

    const RemoteIPSocket remote = from_storage;
    const ChatMessage msg{msg_buffer.data()};
    INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ") from " << remote;

    if (msg == quit_msg)
        return handleGracefulShutdown(remote);

    if (msg == keep_alive_msg)
        send(keep_alive_ack_msg, remote);

    if (peers.count(remote))
        resetTasks(remote);
    else
        return handleCommUp(remote);
}

void SocketUdp::send(const ChatMessage& msg, const RemoteIPSocket& remote)
{
    INFO_LOG << "Sending message: " << msg << " (size = " << msg.size() << ") on fd = " << fd
             << ", remote = " << remote;

    const sockaddr_storage saddr = remote;
    checkSend(sendto(fd, msg.data(), msg.size(), ignore_flags, asSockaddrPtr(saddr), remote.sizeofSockaddr()));
}

void SocketUdp::handleCommUp(const RemoteIPSocket& remote)
{
    INFO_LOG << "New peer " << remote;
    peers.emplace(remote, Peer{schedulePing(remote), scheduleCommLost(remote)});
}

void SocketUdp::handleGracefulShutdown(const RemoteIPSocket& remote)
{
    INFO_LOG << "Graceful shutdown on fd = " << fd << ", peer " << remote;
    remove(remote);
}

void SocketUdp::handleCommLost(const RemoteIPSocket& remote)
{
    INFO_LOG << "No connection on fd = " << fd << " towards " << remote;
    scheduleReestablishment(remote);
    remove(remote);
}

void SocketUdp::remove(const RemoteIPSocket& remote)
{
    disableTasks(remote);
    peers.erase(remote);
    DEBUG_LOG << "Removed peer = " << remote;
}

ScheduledTaskPtr SocketUdp::schedulePing(const RemoteIPSocket& remote)
{
    return task_scheduler.schedule(
        [=, this] { send(keep_alive_msg, remote); }, keep_alive_period, Repetitions{keep_alive_probes});
}

ScheduledTaskPtr SocketUdp::scheduleCommLost(const RemoteIPSocket& remote)
{
    return task_scheduler.schedule([=, this] { handleCommLost(remote); }, keep_alive_period * (keep_alive_probes + 1));
}

void SocketUdp::resetTasks(const RemoteIPSocket& remote)
{
    auto& peer = peers[remote];

    if (auto task = peer.ping_task.lock())
        task->reset();
    else
        peer.ping_task = schedulePing(remote);

    if (auto task = peer.comm_lost_task.lock())
        task->reset();
}

void SocketUdp::disableTasks(const RemoteIPSocket& remote)
{
    DEBUG_LOG << "Disabling tasks for " << remote;
    auto& peer = peers[remote];

    if (auto task = peer.ping_task.lock())
        task->disable();

    if (auto task = peer.comm_lost_task.lock())
        task->disable();
}
