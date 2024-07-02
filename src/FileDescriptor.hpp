#pragma once

#include "Typedefs.hpp"

class FileDescriptor
{
public:
    explicit FileDescriptor(FD = invalid_fd);
    ~FileDescriptor();
    FileDescriptor(FileDescriptor&&) noexcept;
    FileDescriptor& operator=(FileDescriptor&&) noexcept;

    operator FD() const;

    void close();

private:
    static constexpr auto invalid_fd = -1;

    FD fd;
    bool should_close = true;
};
