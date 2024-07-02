#pragma once

#include <unordered_map>
#include "Socket.hpp"

struct NetworkConfiguration;

class SocketSctp : public Socket
{
public:
    SocketSctp(const NetworkConfiguration&, TaskScheduler&);

    void connect(const RemoteIPSockets&) override;
    void connectMultihomed(const RemoteIPSockets&) override;
    void send(const ChatMessage&) override;

private:
    void configure(FD) override;
    void bind(FD, const LocalIPSockets&) override;
    LocalIPSockets getBoundAddresses(FD) const override;
    RemoteIPSockets getPeerAddresses(FD) const override;
    FDs selectFds() override;
    void handleMessage(FD) override;
    IOBuffer& getBuffer(FD) override;

    struct Notification
    {
        IPSocket from;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        sctp_notification sn;
#pragma GCC diagnostic pop
    };

    void sendBigMessage(FD);
    void send(FD, const ChatMessage&);
    void handle(const Notification&);
    void handleCommUp(AssocId);
    void handleEstablishmentFailure(const RemoteIPSocket&);
    void handleGracefulShutdown(AssocId);
    void handleCommLost(AssocId, const RemoteIPSocket&);
    void peelOff(AssocId);
    void remove(AssocId);

    struct Peer
    {
        FileDescriptor fd;
        BufferPtr msg_buffer;
    };
    using Peers = std::unordered_map<AssocId, Peer>;
    Peers peers;
    mutable std::mutex peers_mtx;
    const NetworkConfiguration& config;
};
