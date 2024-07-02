#include "FileDescriptor.hpp"
#include <tools/ThreadSafeLogger.hpp>

using namespace std;

FileDescriptor::FileDescriptor(FD fd) : fd(fd) {}

FileDescriptor::~FileDescriptor()
{
    close();
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept : fd(move(other.fd))
{
    other.should_close = false;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept
{
    if (this != &other)
    {
        close();
        fd = move(other.fd);
        other.should_close = false;
    }
    return *this;
}

FileDescriptor::operator FD() const
{
    return fd;
}

void FileDescriptor::close()
{
    if (should_close and not(fd == invalid_fd))
    {
        DEBUG_LOG << "Closing fd = " << fd;
        ::close(fd);
        should_close = false;
    }
}
