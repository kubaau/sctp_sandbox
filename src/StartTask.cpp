#include "StartTask.hpp"
#include <tools/CountTime.hpp>
#include <tools/ThreadSafeLogger.hpp>

using namespace std;
using namespace chrono;

void startTask(Name name, Delay delay)
{
    threadName(name);

    if (const auto delay_ms = count<milliseconds>(delay))
    {
        DEBUG_LOG << "Task delayed for " << delay_ms << "ms";
    }
    this_thread::sleep_for(delay);

    DEBUG_LOG << "Starting task";
}
