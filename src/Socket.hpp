#pragma once

#include <tools/BufferPool.hpp>
#include "FDSet.hpp"
#include "FileDescriptor.hpp"

class TaskScheduler;

using BacklogCount = int;

constexpr auto default_protocol = 0;
constexpr auto ignore_flags = 0;

class Socket
{
protected:
    using Type = int;
    using Protocol = int;
    using BufferPtr = BufferPool<IOBuffer>::BufferPtr;
    using DeferCreation = bool;

public:
    virtual ~Socket() {}

    Socket(const LocalIPSockets&,
           TaskScheduler&,
           Type,
           Protocol = default_protocol,
           Family = AF_UNSPEC,
           DeferCreation = false);

    void listen(BacklogCount);
    virtual void connect(const RemoteIPSockets&);
    virtual void connectMultihomed(const RemoteIPSockets&);
    virtual void receive();
    virtual void send(const ChatMessage&) = 0;

protected:
    virtual void configure(FD);
    virtual void bind(FD, const LocalIPSockets&);
    virtual LocalIPSockets getBoundAddresses(FD) const;
    virtual RemoteIPSockets getPeerAddresses(FD) const;
    virtual FDs selectFds();
    virtual void handleMessage(FD) = 0;
    virtual IOBuffer& getBuffer(FD);

    bool shouldListen() const;
    void connect(FD, const RemoteIPSocket&);
    std::pair<FileDescriptor, RemoteIPSocket> accept();
    void scheduleReestablishment(const RemoteIPSocket&);

    FileDescriptor fd;
    Family family;
    Type type;

    FDSet fd_set;

    BufferPool<IOBuffer> peer_msg_buffers{64 * 1024};
    BufferPtr main_msg_buffer = peer_msg_buffers.get();

    TaskScheduler& task_scheduler;
};

template <class T>
sockaddr* asSockaddrPtr(T* ptr)
{
    return const_cast<sockaddr*>(reinterpret_cast<const sockaddr*>(ptr));
}

template <class T>
sockaddr* asSockaddrPtr(T& ref)
{
    return asSockaddrPtr(&ref);
}
