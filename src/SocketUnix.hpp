#pragma once

#include <set>
#include "Socket.hpp"

using Path = std::string;

class SocketUnix : public Socket
{
public:
    SocketUnix(TaskScheduler&);
    ~SocketUnix();

    void connect(const RemoteIPSockets&) override;
    void send(const ChatMessage&) override;

private:
    void bind(FD, const LocalIPSockets&) override;
    void handleMessage(FD) override;

    void send(const ChatMessage&, Path);
    void handleCommUp(Path);
    void handleGracefulShutdown(Path);

    std::set<Path> peers;
};
