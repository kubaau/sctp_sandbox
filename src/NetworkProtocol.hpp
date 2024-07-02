#pragma once

#include <tools/Contains.hpp>
#include <tools/EnumToString.hpp>

DEFINE_ENUM_CLASS_WITH_STRING_CONVERSIONS(NetworkProtocol, (SCTP)(TCP)(UDP)(UDP_Lite)(DCCP)(Unix)(UnixForked))

inline auto isUnix(NetworkProtocol protocol)
{
    return in(protocol, {NetworkProtocol::Unix, NetworkProtocol::UnixForked});
}
