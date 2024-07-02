#include "NetworkTask.hpp"
#include <tools/ErrorChecks.hpp>
#include <tools/TaskScheduler.hpp>
#include "Chat.hpp"
#include "Constants.hpp"
#include "Socket.hpp"
#include "SocketFactory.hpp"
#include "StartTask.hpp"

using namespace std;

namespace
{
    auto establishSocket(const NetworkConfiguration& config, TaskScheduler& ts)
    {
        auto socket = createSocket(config, ts);
        socket->listen(BacklogCount{10});
        socket->connect(config.remotes);
        return socket;
    }

    auto chatOrQuit(Socket& socket)
    {
        const auto& msg = chat();
        if (not msg.empty())
        {
            if (msg != quit_msg)
                socket.send(msg);
            else
                chat(msg);
        }
        return msg;
    }
} // namespace

void networkTask(const NetworkTaskParams& p) try
{
    startTask(p.name, p.delay);

    TaskScheduler ts;

    auto socket = establishSocket(p.config, ts);

    const auto should_live_forever = p.lifetime == 0ms;
    if (not should_live_forever)
    {
        DEBUG_LOG << "This task will die in " << p.lifetime.count() << "ms";
    }

    const auto time_to_die = now() + p.lifetime;
    ChatMessage send_msg;
    while ((should_live_forever or now() < time_to_die) and send_msg != quit_msg)
    {
        ts.launch();
        socket->receive();
        send_msg = chatOrQuit(*socket);
    }

    DEBUG_LOG << "Ending task";
}
LOG_EXCEPTIONS
