#pragma once

#include <mqueue.h>
#include <semaphore.h>
#include "Socket.hpp"

class SocketUnixForked : public Socket
{
public:
    using Semaphore = sem_t;

    SocketUnixForked(TaskScheduler&);
    ~SocketUnixForked();

    void send(const ChatMessage&) override;
    void receive() override;

private:
    using FileDescriptorPair = std::pair<FileDescriptor, FileDescriptor>;
    using SocketPairFds = FileDescriptorPair;
    using PipeFds = FileDescriptorPair;
    using NamedPipeFds = FileDescriptorPair;
    using PipeFd = FileDescriptor;
    using NamedPipeFd = FileDescriptor;

    using ProcessId = pid_t;
    using MessageQueue = mqd_t;
    using SharedMemoryId = int;

    struct SharedMemory
    {
        int i;
        Semaphore sem;
    };
    static constexpr auto shm_size = sizeof(SharedMemory);

    void handleMessage(FD) override;

    void createMessageQueues();
    void createPosixSharedMemory();
    void createSysVSharedMemory();
    void createNamedSemaphore();
    void createUnnamedSemaphore();
    FileDescriptorPair createSocketPair();
    FileDescriptorPair createPipe();
    FileDescriptorPair createNamedPipe();
    void fork(SocketPairFds, PipeFds, NamedPipeFds);
    void forkParent(FileDescriptor, PipeFd, NamedPipeFd);
    void forkChild(FileDescriptor, PipeFd, NamedPipeFd);
    void killChildOnParentDeath();
    void checkParentAlive();

    bool isChild() const;

    void cleanUpMessageQueues();
    void cleanUpSemaphores();
    void cleanUpSharedMemory();

    void sendOnSocketPair(const ChatMessage&);
    void sendOnPipe(const ChatMessage&);
    void sendOnNamedPipe(const ChatMessage&);
    void sendOnMessageQueue(const ChatMessage&);
    void setPosixSharedMemory(int);
    void postNamedSemaphore();
    void setSysVSharedMemory(int);
    void postUnnamedSemaphore();

    void readPosixSharedMemory();
    void readSysVSharedMemory();
    void receiveFromPipe();
    void receiveFromNamedPipe();
    void receiveFromMessageQueue();

    void printSemaphoreValues() const;

    ProcessId parent;
    FileDescriptor pipe;
    FileDescriptor named_pipe;
    MessageQueue mq;
    MessageQueue peer_mq;
    FileDescriptor shm;
    SharedMemoryId shm_id;
    SharedMemory* shm_mmap;
    SharedMemory* shm_shmat;
    Semaphore* sem_named;
    Semaphore* sem_unnamed;
};
