#pragma once

#include <tools/TaskScheduler.hpp>
#include "Socket.hpp"

class SocketUdp : public Socket
{
public:
    SocketUdp(const LocalIPSockets&, TaskScheduler&, Protocol = default_protocol);
    ~SocketUdp();

    void connect(const RemoteIPSockets&) override;
    void send(const ChatMessage&) override;

private:
    void handleMessage(FD) override;

    void send(const ChatMessage&, const RemoteIPSocket&);
    void handleCommUp(const RemoteIPSocket&);
    void handleGracefulShutdown(const RemoteIPSocket&);
    void handleCommLost(const RemoteIPSocket&);
    void remove(const RemoteIPSocket&);

    ScheduledTaskPtr schedulePing(const RemoteIPSocket&);
    ScheduledTaskPtr scheduleCommLost(const RemoteIPSocket&);
    void resetTasks(const RemoteIPSocket&);
    void disableTasks(const RemoteIPSocket&);

    struct Peer
    {
        ScheduledTaskPtr ping_task;
        ScheduledTaskPtr comm_lost_task;
    };
    using Peers = std::map<RemoteIPSocket, Peer>;
    Peers peers;

    IP local;
};
