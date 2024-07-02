#pragma once

#include "Constants.hpp"
#include "Typedefs.hpp"

IPSockets getLaddrs(FD, AssocId = ignore_assoc);
IPSockets getPaddrs(FD, AssocId = ignore_assoc);
