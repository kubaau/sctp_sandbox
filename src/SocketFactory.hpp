#pragma once

#include <memory>
#include "NetworkConfiguration.hpp"

class Socket;
class TaskScheduler;
std::unique_ptr<Socket> createSocket(const NetworkConfiguration&, TaskScheduler&);
