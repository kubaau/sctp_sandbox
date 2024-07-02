#include <tools/AsyncTask.hpp>
#include <tools/ContainerOperators.hpp>
#include <tools/ErrorChecks.hpp>
#include <tools/PrintBacktrace.hpp>
#include <tools/ThreadSafeLogger.hpp>
#include "IoTask.hpp"
#include "NetworkTask.hpp"

using namespace std;
using namespace RangeOperators;

namespace
{
    void runPredefinedConfig(const NetworkConfiguration& config, AsyncTasks& tasks)
    {
        constexpr auto localhost = "127.0.0.1";
        // constexpr auto nat = "10.0.2.15";
        constexpr auto s1_port = 36412;
        constexpr auto x2_port = 36422;

        auto mme = config;
        mme.locals += IPSocket{localhost, s1_port};
        // mme.locals += IPSocket{nat, s1_port};

        auto enb1 = config;
        enb1.locals += IPSocket{localhost, x2_port};
        enb1.remotes += mme.locals.front();

        auto enb2 = config;
        enb2.locals += IPSocket{localhost, x2_port + 1};
        // enb2.locals += IPSocket{nat, x2_port + 1};
        enb2.remotes += mme.locals.front();
        enb2.remotes += enb1.locals;
        enb2.remotes.back().port += 17;

        tasks += asyncTask(networkTask, NetworkTaskParams{mme, "MME"});
        tasks += asyncTask(networkTask, NetworkTaskParams{enb1, "eNB1", Delay{100ms}});
        tasks += asyncTask(networkTask, NetworkTaskParams{enb2, "eNB2", Delay{200ms}, Lifetime{13s}});
    }

    void runNetwork(const NetworkConfiguration& config, AsyncTasks& tasks)
    {
        if (not config.locals.empty() or isUnix(config.protocol))
            tasks += asyncTask(networkTask, NetworkTaskParams{config});
        else
            runPredefinedConfig(config, tasks);
    }
} // namespace

int main(int argc, char* argv[]) try
{
    installSigaction({SIGTERM, SIGSEGV}, printBacktrace);

    const auto args = readArgs(argc, argv);
    // if (contains(args, "-debug"))
    enableDebugLogs(EnableDebugLogs::YES);
    DEBUG_LOG << "Args: " << args;

    AsyncTasks tasks;
    tasks += asyncTask(ioTask);
    runNetwork(args, tasks);
    join(tasks);
}
LOG_EXCEPTIONS
