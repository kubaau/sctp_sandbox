#pragma once

#include <map>
#include "Socket.hpp"

class SocketTcp : public Socket
{
public:
    SocketTcp(const LocalIPSockets&, TaskScheduler&, Type = SOCK_STREAM);

    void connect(const RemoteIPSockets&) override;
    void send(const ChatMessage&) override;

protected:
    void configure(FD) override;
    FDs selectFds() override;
    void handleMessage(FD) override;
    IOBuffer& getBuffer(FD) override;

    using ShouldReestablish = bool;
    struct Peer
    {
        FileDescriptor fd;
        BufferPtr msg_buffer;
        RemoteIPSocket remote;
        ShouldReestablish should_reestablish;
    };

    FileDescriptor createConnectSocket();
    void sendMessage(const ChatMessage&, const Peer&);
    void receiveMessage(FD);
    void handleCommUp();
    void handleGracefulShutdown(FD);
    void handleCommLost(FD);
    void remove(FD);
    using Peers = std::map<FD, Peer>;
    Peers peers;
    mutable std::mutex peers_mtx;

    IP local;
    Type type;
    Protocol protocol;
};
