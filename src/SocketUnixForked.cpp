#include "SocketUnixForked.hpp"
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <tools/RandomContainers.hpp>
#include <tools/Sigaction.hpp>
#include "SocketConfiguration.hpp"

using namespace std;

namespace
{
    using FdPair = pair<FD, FD>;

    constexpr auto protocol = default_protocol;
    constexpr auto parent_mq_name = "/parent";
    constexpr auto child_mq_name = "/child";
    constexpr auto pipe_flags = O_CLOEXEC | O_NONBLOCK;
    constexpr auto pipe_name = "/tmp/named_pipe";
    constexpr auto permissions = 0666;
    constexpr auto ignore_mapping_hint = nullptr;
    constexpr auto shm_name = "/shm";
    constexpr auto sem_name = "/sem";
    constexpr auto initial_sem_value = 0;

    SigActionSignature(childDied) { throw runtime_error{"Oh no, my child is dead ;("}; }
    SigActionSignature(parentDied) { throw runtime_error{"Oh no, my parent is dead ;("}; }
}

SocketUnixForked::SocketUnixForked(TaskScheduler& ts)
    : Socket(LocalIPSockets{}, ts, SOCK_DGRAM, protocol, AF_UNIX, DeferCreation{true})
{
    createMessageQueues();
    createPosixSharedMemory();
    createSysVSharedMemory();
    createNamedSemaphore();
    createUnnamedSemaphore();
    fork(createSocketPair(), createPipe(), createNamedPipe());
    configure(fd);
    DEBUG_LOG << "Configured fd = " << fd;
}

SocketUnixForked::~SocketUnixForked()
{
    cleanUpMessageQueues();
    cleanUpSemaphores();
    cleanUpSharedMemory();

    if (isChild())
    {
        INFO_LOG << "Child is dying - notifying parent " << parent;
        kill(parent, SIGCHLD);
    }
}

void SocketUnixForked::send(const ChatMessage& msg)
{
    sendOnSocketPair(msg);
    sendOnPipe(msg);
    sendOnNamedPipe(msg);
    sendOnMessageQueue(msg);
    setPosixSharedMemory(msg.size());
    postNamedSemaphore();
    setSysVSharedMemory(msg.size());
    postUnnamedSemaphore();
}

void SocketUnixForked::receive()
{
    Socket::receive();

    readPosixSharedMemory();
    readSysVSharedMemory();

    if (isChild())
    {
        receiveFromPipe();
        receiveFromNamedPipe();
    }

    receiveFromMessageQueue();
}

void SocketUnixForked::handleMessage(FD fd)
{
    auto& msg_buffer = getBuffer(fd);
    msg_buffer[checkedReceive(recv(fd, msg_buffer.data(), msg_buffer.size(), ignore_flags))] = 0;

    const ChatMessage msg{msg_buffer.data()};
    INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ")";

    if (msg == quit_msg)
        throw runtime_error{"Unwinding stack"};
}

SocketUnixForked::FileDescriptorPair SocketUnixForked::createSocketPair()
{
    FdPair fds;
    checkSocketpair(socketpair(family, type, protocol, &fds.first));
    return FileDescriptorPair{fds.first, fds.second};
}

void SocketUnixForked::createMessageQueues()
{
    constexpr auto mq_flags = O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK | O_CLOEXEC;
    constexpr auto mode = S_IRUSR | S_IWUSR;
    constexpr auto default_attr = nullptr;
    mq_unlink(parent_mq_name);
    mq_unlink(child_mq_name);
    mq = checkedMqOpen(mq_open(parent_mq_name, mq_flags, mode, default_attr));
    peer_mq = checkedMqOpen(mq_open(child_mq_name, mq_flags, mode, default_attr));
}

SocketUnixForked::FileDescriptorPair SocketUnixForked::createPipe()
{
    FdPair fds;
    checkPipe(pipe2(&fds.first, pipe_flags | O_DIRECT));
    return FileDescriptorPair{fds.first, fds.second};
}

SocketUnixForked::FileDescriptorPair SocketUnixForked::createNamedPipe()
{
    checkRemove(remove(pipe_name));
    checkMkfifo(mkfifo(pipe_name, permissions));
    return FileDescriptorPair{checkedOpen(open(pipe_name, O_RDONLY | pipe_flags)),
                              checkedOpen(open(pipe_name, O_WRONLY | pipe_flags))};
}

void SocketUnixForked::createPosixSharedMemory()
{
    shm = FileDescriptor{checkedShmopen(shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, permissions))};
    checkTruncate(ftruncate(shm, shm_size));
    constexpr auto offset = 0;
    shm_mmap = reinterpret_cast<SharedMemory*>(
        checkedMmap(mmap(ignore_mapping_hint, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, offset)));
}

void SocketUnixForked::createSysVSharedMemory()
{
    shm_id = checkedShmget(shmget(IPC_PRIVATE, shm_size, permissions | IPC_CREAT));
    constexpr auto shmat_flags = 0;
    shm_shmat = reinterpret_cast<SharedMemory*>(checkedShmat(shmat(shm_id, ignore_mapping_hint, shmat_flags)));
}

void SocketUnixForked::createNamedSemaphore()
{
    sem_named = checkedSemopen(sem_open(sem_name, O_CREAT | O_EXCL, permissions, initial_sem_value));
}

void SocketUnixForked::createUnnamedSemaphore()
{
    sem_unnamed = &shm_mmap->sem;
    constexpr auto shared_between_processes = true;
    checkSeminit(sem_init(sem_unnamed, shared_between_processes, initial_sem_value));
}

void SocketUnixForked::fork(SocketPairFds socket_pair_fds, PipeFds pipe_fds, NamedPipeFds named_pipe_fds)
{
    const auto pid = getpid();
    DEBUG_LOG DEBUG_VAR(pid);

    if (const auto fork_result = ::fork())
    {
        DEBUG_LOG DEBUG_VAR(checkedFork(fork_result));
        // pipefd[1] refers to the write end of the pipe.  pipefd[0] refers to the read end of the pipe.
        forkParent(move(socket_pair_fds.second), move(pipe_fds.second), move(named_pipe_fds.second));
    }
    else
    {
        parent = pid;
        forkChild(move(socket_pair_fds.first), move(pipe_fds.first), move(named_pipe_fds.first));
    }
}

static void killParentOnChildDeath()
{
    installSigaction(SIGCHLD, childDied);
}

void SocketUnixForked::forkParent(FileDescriptor fd, PipeFd pipe, NamedPipeFd named_pipe)
{
    threadName("parent");
    killParentOnChildDeath();
    this->fd = move(fd);
    this->pipe = move(pipe);
    this->named_pipe = move(named_pipe);
}

void SocketUnixForked::forkChild(FileDescriptor fd, PipeFd pipe, NamedPipeFd named_pipe)
{
    threadName("child");
    killChildOnParentDeath();
    this->fd = move(fd);
    this->pipe = move(pipe);
    this->named_pipe = move(named_pipe);
    swap(mq, peer_mq);
}

void SocketUnixForked::killChildOnParentDeath()
{
    installSigaction(SIGTERM, parentDied);
    checkPrctl(prctl(PR_SET_PDEATHSIG, SIGTERM));
    checkParentAlive();
}

void SocketUnixForked::checkParentAlive()
{
    const auto current_ppid = getppid();
    if (current_ppid != parent)
    {
        ostringstream oss;
        oss << "Parent died during child creation :( " DEBUG_VAR(parent) DEBUG_VAR(current_ppid);
        throw runtime_error{oss.str()};
    }
}

bool SocketUnixForked::isChild() const
{
    return parent;
}

void SocketUnixForked::cleanUpMessageQueues()
{
    mq_close(mq);
    mq_close(peer_mq);
    mq_unlink(parent_mq_name);
    mq_unlink(child_mq_name);
}

void SocketUnixForked::cleanUpSemaphores()
{
    sem_close(sem_named);
    sem_unlink(sem_name);
    sem_close(sem_unnamed);
    sem_destroy(sem_unnamed);
}

void SocketUnixForked::cleanUpSharedMemory()
{
    munmap(shm_mmap, shm_size);
    shm_unlink(shm_name);
    shmdt(shm_shmat);
}

void SocketUnixForked::sendOnSocketPair(const ChatMessage& msg)
{
    INFO_LOG << "Sending message: " << msg << " (size = " << msg.size() << ") on " DEBUG_VAR(fd);
    checkSend(::send(fd, msg.data(), msg.size(), ignore_flags));
}

void SocketUnixForked::sendOnPipe(const ChatMessage& msg)
{
    INFO_LOG << "Sending message: " << msg << " (size = " << msg.size() << ") on " DEBUG_VAR(pipe);
    checkWrite(write(pipe, msg.data(), msg.size()));
}

void SocketUnixForked::sendOnNamedPipe(const ChatMessage& msg)
{
    INFO_LOG << "Sending message: " << msg << " (size = " << msg.size() << ") on " DEBUG_VAR(named_pipe);
    checkWrite(write(named_pipe, msg.data(), msg.size()));
}

void SocketUnixForked::sendOnMessageQueue(const ChatMessage& msg)
{
    for (auto prio : shuffledIndexes(5))
    {
        INFO_LOG << "Sending message: " << msg << " (size = " << msg.size() << ") with " DEBUG_VAR(prio);
        checkMqSend(mq_send(peer_mq, msg.data(), msg.size(), prio));
    }
}

void SocketUnixForked::setPosixSharedMemory(int v)
{
    INFO_LOG << "Setting mmap shared integer to: " << v;
    shm_mmap->i = v;
}

void SocketUnixForked::postNamedSemaphore()
{
    checkSempost(sem_post(sem_named));
    printSemaphoreValues();
}

void SocketUnixForked::setSysVSharedMemory(int v)
{
    INFO_LOG << "Setting shmat shared integer to: " << v;
    shm_shmat->i = v;
}

void SocketUnixForked::postUnnamedSemaphore()
{
    checkSempost(sem_post(sem_unnamed));
    printSemaphoreValues();
}

static bool isSemaphoreReady(SocketUnixForked::Semaphore& sem)
{
    return checkedSemtrywait(sem_trywait(&sem)) == 0;
}

void SocketUnixForked::readPosixSharedMemory()
{
    if (isSemaphoreReady(*sem_named))
    {
        INFO_LOG << "Shared mmap integer is: " << shm_mmap->i;
        printSemaphoreValues();
    }
}

void SocketUnixForked::readSysVSharedMemory()
{
    if (isSemaphoreReady(*sem_unnamed))
    {
        INFO_LOG << "Shared shmat integer is: " << shm_shmat->i;
        printSemaphoreValues();
    }
}

void SocketUnixForked::receiveFromPipe()
{
    auto& msg_buffer = *main_msg_buffer;
    const auto result = checkedRead(read(pipe, msg_buffer.data(), msg_buffer.size()));
    if (result > 0)
    {
        msg_buffer[result] = 0;
        const ChatMessage msg{msg_buffer.data()};
        INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ") on " DEBUG_VAR(pipe);
    }
}

void SocketUnixForked::receiveFromNamedPipe()
{
    auto& msg_buffer = *main_msg_buffer;
    const auto result = checkedRead(read(named_pipe, msg_buffer.data(), msg_buffer.size()));
    if (result > 0)
    {
        msg_buffer[result] = 0;
        const ChatMessage msg{msg_buffer.data()};
        INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ") on " DEBUG_VAR(named_pipe);
    }
}

void SocketUnixForked::receiveFromMessageQueue()
{
    this_thread::sleep_for(10ms);

    auto& msg_buffer = *main_msg_buffer;
    unsigned prio;
    const auto result = checkedMqReceive(mq_receive(mq, msg_buffer.data(), msg_buffer.size(), &prio));
    if (result > 0)
    {
        msg_buffer[result] = 0;
        const ChatMessage msg{msg_buffer.data()};
        INFO_LOG << "Received message: " << msg << " (size = " << msg.size() << ") with " DEBUG_VAR(prio);
    }
}

void SocketUnixForked::printSemaphoreValues() const
{
    int value;
    checkSemgetvalue(sem_getvalue(sem_named, &value));
    DEBUG_LOG << "sem_named = " << value;
    checkSemgetvalue(sem_getvalue(sem_unnamed, &value));
    DEBUG_LOG << "sem_unnamed = " << value;
}
