#include "SocketErrorChecks.hpp"
#include <semaphore.h>
#include <sys/mman.h>
#include <tools/Contains.hpp>

bool failedAccept(int result)
{
    return result < 0;
}

bool failedBind(int result)
{
    return result != 0;
}

bool failedConnect(int result)
{
    return not(result == 0 or in(errno, {EINPROGRESS, EINTR}));
}

bool failedFcntl(int result)
{
    return result == -1;
}

bool failedFork(int result)
{
    return result < 0;
}

bool failedGetaddrs(int result)
{
    return result < 0;
}

bool failedGetpeername(int result)
{
    return result != 0;
}

bool failedGetsockname(int result)
{
    return result != 0;
}

bool failedGetsockopt(int result)
{
    return result != 0;
}

bool failedInetpton(int result)
{
    return result != 1;
}

bool failedListen(int result)
{
    return result != 0;
}

bool failedMkfifo(int result)
{
    return result != 0;
}

bool failedMqClose(int result)
{
    return result != 0;
}

bool failedMqOpen(int result)
{
    return result < 0;
}

bool failedMqReceive(int result)
{
    return result < 0 and errno != EAGAIN;
}

bool failedMqSend(int result)
{
    return result != 0;
}

bool failedMqUnlink(int result)
{
    return result != 0;
}

bool failedOpen(int result)
{
    return result < 0;
}

bool failedOptinfo(int result)
{
    return result != 0;
}

bool failedPeeloff(int result)
{
    return result < 0;
}

bool failedPipe(int result)
{
    return result != 0;
}

bool failedPrctl(int result)
{
    return result == -1;
}

bool failedRead(int result)
{
    return result < 0 and errno != EAGAIN;
}

bool failedReceive(int result)
{
    return result < 0;
}

bool failedRemove(int result)
{
    return result != 0 and errno != ENOENT;
}

bool failedSelect(int result)
{
    return result < 0;
}

bool failedSemgetvalue(int result)
{
    return result != 0;
}

bool failedSeminit(int result)
{
    return result != 0;
}

bool failedSempost(int result)
{
    return result != 0;
}

bool failedSemtrywait(int result)
{
    return result != 0 and errno != EAGAIN;
}

bool failedSend(int result)
{
    return result < 0;
}

bool failedSetsockopt(int result)
{
    return result != 0;
}

bool failedShmdt(int result)
{
    return result != 0;
}

bool failedShmget(int result)
{
    return result < 0;
}

bool failedShmopen(int result)
{
    return result < 0;
}

bool failedSocket(int result)
{
    return result < 0;
}

bool failedSocketpair(int result)
{
    return result != 0;
}

bool failedTruncate(int result)
{
    return result != 0;
}

bool failedWrite(int result)
{
    return result < 0;
}

bool failedWsaStartup(int result)
{
    return result != 0;
}

bool failedInetntop(const void* result)
{
    return not result;
}

bool failedMmap(const void* result)
{
    return result == MAP_FAILED;
}

bool failedSemopen(const void* result)
{
    return result == SEM_FAILED;
}

bool failedShmat(const void* result)
{
    return result == MAP_FAILED;
}
