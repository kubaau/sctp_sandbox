#include "SocketUnix.hpp"
#include <tools/ThreadSafeLogger.hpp>
#include "Constants.hpp"
#include "SocketConfiguration.hpp"
#include "SocketErrorChecks.hpp"

using namespace std;

SocketUnix::SocketUnix(TaskScheduler& ts)
    : Socket(LocalIPSockets{}, ts, SOCK_DGRAM, default_protocol, AF_UNIX)
{
    SocketUnix::configure(fd);
    DEBUG_LOG << "Configured fd = " << fd;
    SocketUnix::bind(fd, {});
}

SocketUnix::~SocketUnix()
{
    SocketUnix::send(quit_msg);
}

void SocketUnix::connect(const RemoteIPSockets& remotes)
{
    for (const auto& remote : remotes)
        handleCommUp(remote.addr);
}

void SocketUnix::send(const ChatMessage& msg)
{
    for (auto p : peers)
        send(msg, p);
}

void SocketUnix::bind(FD fd, const LocalIPSockets&)
{
    sockaddr_un saddr{};
    saddr.sun_family = AF_UNIX;
    checkBind(::bind(fd, asSockaddrPtr(saddr), sizeof(Family)));

    socklen_t saddr_len = sizeof(saddr);
    checkGetsockname(getsockname(fd, asSockaddrPtr(saddr), &saddr_len));
    saddr.sun_path[0] = '#';
    INFO_LOG << "Bound socket: fd = " << fd << ", path = " << saddr.sun_path;
}

void SocketUnix::handleMessage(FD fd)
{
    sockaddr_storage from_storage{};
    socklen_t from_len = sizeof(from_storage);

    auto& msg_buffer = getBuffer(fd);

    const auto receive_result =
        recvfrom(fd, msg_buffer.data(), msg_buffer.size(), ignore_flags, asSockaddrPtr(from_storage), &from_len);
    checkReceive(receive_result);
    msg_buffer[receive_result] = 0;

    auto& sun_path = reinterpret_cast<sockaddr_un&>(from_storage).sun_path;
    sun_path[0] = '#';

    const ChatMessage msg{msg_buffer.data()};
    INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ") from " << sun_path;

    const Path path = sun_path + 1;

    if (msg == quit_msg)
        return handleGracefulShutdown(path);

    if (not peers.count(path))
        return handleCommUp(path);
}

void SocketUnix::send(const ChatMessage& msg, Path path)
{
    sockaddr_un saddr{};
    saddr.sun_family = family;
    copy(cbegin(path), cend(path), saddr.sun_path + 1);

    saddr.sun_path[0] = '#';
    INFO_LOG << "Sending message: " << msg << " (size = " << msg.size() << ") on fd = " << fd
             << ", path = " << saddr.sun_path;
    saddr.sun_path[0] = 0;
    checkSend(
        sendto(fd, msg.data(), msg.size(), ignore_flags, asSockaddrPtr(saddr), sizeof(Family) + path.length() + 1));
}

void SocketUnix::handleCommUp(Path path)
{
    INFO_LOG << "New path " << path;
    peers.insert(path);
}

void SocketUnix::handleGracefulShutdown(Path path)
{
    INFO_LOG << "Graceful shutdown on fd = " << fd << ", path " << path;
    peers.erase(path);
}
