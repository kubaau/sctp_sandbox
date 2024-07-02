#include "SocketDccp.hpp"
#include <tools/ThreadSafeLogger.hpp>
#include "Constants.hpp"

using namespace std;

SocketDccp::SocketDccp(const LocalIPSockets& locals, TaskScheduler& ts) : SocketTcp(locals, ts, SOCK_DCCP) {}

SocketDccp::~SocketDccp()
{
    SocketDccp::send(quit_msg);
}

void SocketDccp::send(const ChatMessage& msg)
{
    LOCK_MTX(peers_mtx);
    for (const auto& p : peers)
        try
        {
            sendMessage(msg, p.second);
        }
        catch (const runtime_error& ex)
        {
            WARN_LOG << ex.what();
        }
}

void SocketDccp::handleMessage(FD fd)
{
    if (fd == this->fd)
        return handleCommUp();

    receiveMessage(fd);
    const ChatMessage msg{getBuffer(fd).data()};

    if (msg.empty())
        return handleCommLost(fd);

    if (msg == quit_msg)
        return handleGracefulShutdown(fd);

    INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ") from " << getPeerAddresses(fd);
}
