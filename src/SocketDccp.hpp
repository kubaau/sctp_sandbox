#pragma once

#include "SocketTcp.hpp"

class SocketDccp : public SocketTcp
{
public:
    SocketDccp(const LocalIPSockets&, TaskScheduler&);
    ~SocketDccp();

    void send(const ChatMessage&) override;

private:
    void handleMessage(FD) override;
};
