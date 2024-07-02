#include "SocketTcp.hpp"
#include "SocketConfiguration.hpp"
#include "SocketErrorChecks.hpp"
#include "SocketIO.hpp"

using namespace std;

SocketTcp::SocketTcp(const LocalIPSockets& locals, TaskScheduler& ts, Type type)
    : Socket(locals, ts, type), local(locals.front().addr), type(type)
{
    SocketTcp::configure(fd);
    DEBUG_LOG << "Configured fd = " << fd;
    SocketTcp::bind(fd, locals);
}

void SocketTcp::connect(const RemoteIPSockets& remotes)
{
    LOCK_MTX(peers_mtx);
    for (const auto& remote : remotes)
    {
        auto fd = createConnectSocket();
        Socket::connect(fd, remote);
        peers.emplace(fd, Peer{move(fd), peer_msg_buffers.get(), remote, ShouldReestablish{true}});
    }
}

void SocketTcp::send(const ChatMessage& msg)
{
    LOCK_MTX(peers_mtx);
    for (const auto& p : peers)
        sendMessage(msg, p.second);
}

void SocketTcp::configure(FD fd)
{
    Socket::configure(fd);
    if (type == SOCK_STREAM)
        configureKeepAlive(fd, KeepAliveTimeInS{1}, KeepAliveIntervalInS{1}, KeepAliveProbes{3});
}

FDs SocketTcp::selectFds()
{
    LOCK_MTX(peers_mtx);
    fd_set.reset();
    fd_set.set(fd);
    for (const auto& peer : peers)
        fd_set.set(peer.second.fd);
    return fd_set.select();
}

void SocketTcp::handleMessage(FD fd)
{
    if (fd == this->fd)
        return handleCommUp();

    try
    {
        receiveMessage(fd);
    }
    catch (const runtime_error& ex)
    {
        WARN_LOG << ex.what();
        return handleCommLost(fd);
    }

    const ChatMessage msg{getBuffer(fd).data()};

    if (msg.empty())
        return handleGracefulShutdown(fd);

    INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ") from " << getPeerAddresses(fd);
}

IOBuffer& SocketTcp::getBuffer(FD fd)
{
    LOCK_MTX(peers_mtx);
    return *peers.at(fd).msg_buffer;
}

FileDescriptor SocketTcp::createConnectSocket()
{
    FileDescriptor fd{checkedSocket(socket(family, type, protocol))};
    DEBUG_LOG << "Created socket: fd = " << fd << ", family = " << toString(family) << " for a new connection";
    configure(fd);
    bind(fd, IPSockets{IPSocket{local}});
    return fd;
}

void SocketTcp::sendMessage(const ChatMessage& msg, const SocketTcp::Peer& peer)
{
    const FD fd = peer.fd;
    INFO_LOG << "Sending message: " << msg << " (size = " << msg.size() << ") on fd = " << fd
             << ", remote = " << peer.remote;
    checkSend(::send(fd, msg.data(), msg.size(), ignore_flags));
}

void SocketTcp::receiveMessage(FD fd)
{
    auto& msg_buffer = getBuffer(fd);
    msg_buffer[checkedReceive(recv(fd, msg_buffer.data(), msg_buffer.size(), ignore_flags))] = 0;
}

void SocketTcp::handleCommUp()
{
    LOCK_MTX(peers_mtx);
    auto accept_result = accept();
    auto& fd = accept_result.first;
    const auto& remote = accept_result.second;
    peers.emplace(FD{fd}, Peer{move(fd), peer_msg_buffers.get(), remote, ShouldReestablish{false}});
}

void SocketTcp::handleGracefulShutdown(FD fd)
{
    LOCK_MTX(peers_mtx);
    INFO_LOG << "Graceful shutdown on fd = " << fd << ", peer " << peers.at(fd).remote;
    remove(fd);
}

void SocketTcp::handleCommLost(FD fd)
{
    LOCK_MTX(peers_mtx);
    const auto& peer = peers.at(fd);
    INFO_LOG << "No connection on fd = " << fd << " towards " << peer.remote;
    if (peer.should_reestablish)
        scheduleReestablishment(peer.remote);
    remove(fd);
}

void SocketTcp::remove(FD fd)
{
    LOCK_MTX(peers_mtx);
    peers.erase(fd);
    DEBUG_LOG << "Removed fd = " << fd;
}
