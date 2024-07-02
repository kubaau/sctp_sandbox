#pragma once

#include "Constants.hpp"
#include "SocketErrorChecks.hpp"
#include "Typedefs.hpp"

using InitMaxAttempts = SocketParam;
using InitMaxTimeoutInMs = SocketParam;
using RtoInitialInMs = SocketParam;
using RtoMinInMs = SocketParam;
using RtoMaxInMs = SocketParam;
using SackFreq = SocketParam;
using SackDelay = SocketParam;
using HbInterval = SocketParam;
using PathMaxRetrans = SocketParam;
using PathMtu = SocketParam;
using KeepAliveTimeInS = SocketParam;
using KeepAliveIntervalInS = SocketParam;
using KeepAliveProbes = SocketParam;

constexpr auto all_associations = ignore_assoc;

void configureNoDelay(FD);
void configureReuseAddr(FD);
void configureReceivingEvents(FD);
void configureInitParams(FD, InitMaxAttempts, InitMaxTimeoutInMs);
void configureRto(FD, RtoInitialInMs, RtoMinInMs, RtoMaxInMs, AssocId = all_associations);
void configureSack(FD, SackFreq, SackDelay, AssocId = all_associations);
void configureMaxRetrans(FD, SocketParam, AssocId = all_associations);
void configureDscp(FD, Byte, Family);
void configureIPv4AddressMapping(FD);
void configureTTL(FD, SocketParam);
void configurePeerAddressParams(FD, HbInterval, PathMaxRetrans, PathMtu, AssocId = all_associations);
void configureMaxSeg(FD, SocketParam, AssocId = all_associations);
void configureNonBlockingMode(FD);
void configureKeepAlive(FD, KeepAliveTimeInS, KeepAliveIntervalInS, KeepAliveProbes);
void configurePassCred(FD);

template <class ValueContainer>
void optInfo(FD fd, int option, ValueContainer& value_container, AssocId assoc_id = ignore_assoc)
{
    socklen_t len = sizeof(ValueContainer);
    checkOptinfo(sctp_opt_info(fd, assoc_id, option, &value_container, &len));
}

template <class ValueContainer>
auto getsockopt(FD fd, int protocol, int option, ValueContainer& value_container)
{
    socklen_t len = sizeof(ValueContainer);
    checkGetsockopt(getsockopt(fd, protocol, option, &value_container, &len));
}
