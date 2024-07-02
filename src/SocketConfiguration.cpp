#include "SocketConfiguration.hpp"

namespace
{
    constexpr auto yes = 1;

#define SETSOCKOPT(Protocol, Option, Value) checkSetsockopt(setsockopt(fd, Protocol, Option, &Value, sizeof(Value)))
#define SETSOCKOPT_SCTP(Option, Value) SETSOCKOPT(IPPROTO_SCTP, Option, Value)
#define SETSOCKOPT_SOL(Option, Value) SETSOCKOPT(SOL_SOCKET, Option, Value)
#define SETSOCKOPT_IP(Option, Value) SETSOCKOPT(IPPROTO_IP, Option, Value)
#define SETSOCKOPT_IPV6(Option, Value) SETSOCKOPT(IPPROTO_IPV6, Option, Value)
#define SETSOCKOPT_SOL_SCTP(Option, Value) SETSOCKOPT(SOL_SCTP, Option, Value)
#define SETSOCKOPT_SOL_TCP(Option, Value) SETSOCKOPT(SOL_TCP, Option, Value)
} // namespace

void configureNoDelay(FD fd)
{
    SETSOCKOPT_SCTP(SCTP_NODELAY, yes);
}

void configureReuseAddr(FD fd)
{
    SETSOCKOPT_SOL(SO_REUSEADDR, yes);
}

void configureReceivingEvents(FD fd)
{
    sctp_event_subscribe subscribe{};
    subscribe.sctp_data_io_event = true;
    subscribe.sctp_association_event = true;
    subscribe.sctp_address_event = true;
    subscribe.sctp_send_failure_event = true;
    subscribe.sctp_peer_error_event = true;
    subscribe.sctp_shutdown_event = true;
    subscribe.sctp_partial_delivery_event = true;
    subscribe.sctp_adaptation_layer_event = true;
    SETSOCKOPT_SCTP(SCTP_EVENTS, subscribe);
}

void configureInitParams(FD fd, InitMaxAttempts init_max_attempts, InitMaxTimeoutInMs init_max_timeout)
{
    sctp_initmsg init_msg{};
    init_msg.sinit_max_attempts = init_max_attempts;
    init_msg.sinit_max_init_timeo = init_max_timeout;
    SETSOCKOPT_SCTP(SCTP_INITMSG, init_msg);
}

void configureRto(FD fd, RtoInitialInMs initial, RtoMinInMs min, RtoMaxInMs max, AssocId assoc)
{
    sctp_rtoinfo rto{};
    rto.srto_assoc_id = assoc;
    rto.srto_initial = initial;
    rto.srto_min = min;
    rto.srto_max = max;
    SETSOCKOPT_SCTP(SCTP_RTOINFO, rto);
}

void configureSack(FD fd, SackFreq freq, SackDelay delay, AssocId assoc)
{
    sctp_sack_info sack{};
    sack.sack_assoc_id = assoc;
    sack.sack_freq = freq;
    sack.sack_delay = delay;
    SETSOCKOPT_SCTP(SCTP_DELAYED_SACK, sack);
}

void configureMaxRetrans(FD fd, SocketParam max_retrans, AssocId assoc)
{
    sctp_assocparams assoc_params{};
    assoc_params.sasoc_assoc_id = assoc;
    assoc_params.sasoc_asocmaxrxt = max_retrans;
    SETSOCKOPT_SCTP(SCTP_ASSOCINFO, assoc_params);
}

void configureDscp(FD fd, Byte dscp, Family family)
{
    // DSCP represents the first 6 bits of an 8 bit ToS field - the last 2 bits are ECN-related
    const int tos = dscp << 2;
    family == AF_INET ? SETSOCKOPT_IP(IP_TOS, tos) : SETSOCKOPT_IPV6(IPV6_TCLASS, tos);
}

void configureIPv4AddressMapping(FD fd)
{
    SETSOCKOPT_SOL_SCTP(SCTP_I_WANT_MAPPED_V4_ADDR, yes);
}

void configureTTL(FD fd, SocketParam ttl)
{
    SETSOCKOPT_IP(IP_TTL, ttl);
}

void configurePeerAddressParams(
    FD fd, HbInterval hb_interval, PathMaxRetrans path_max_retrans, PathMtu path_mtu, AssocId assoc)
{
    sctp_paddrparams peer_addr_params{};
    peer_addr_params.spp_assoc_id = assoc;
    peer_addr_params.spp_hbinterval = hb_interval;
    peer_addr_params.spp_pathmaxrxt = path_max_retrans;
    peer_addr_params.spp_pathmtu = path_mtu;
    peer_addr_params.spp_flags =
        (path_mtu ? SPP_PMTUD_DISABLE : SPP_PMTUD_ENABLE) | (hb_interval ? SPP_HB_ENABLE : SPP_HB_DISABLE);
    SETSOCKOPT_SCTP(SCTP_PEER_ADDR_PARAMS, peer_addr_params);
}

void configureMaxSeg(FD fd, SocketParam max_seg, AssocId assoc)
{
    sctp_assoc_value assoc_val{};
    assoc_val.assoc_id = assoc;
    assoc_val.assoc_value = max_seg;
    SETSOCKOPT_SCTP(SCTP_MAXSEG, assoc_val);
}

void configureNonBlockingMode(FD fd)
{
    int flags = 0;
    checkFcntl(flags = fcntl(fd, F_GETFL, flags));
    flags |= O_NONBLOCK;
    checkFcntl(fcntl(fd, F_SETFL, flags));
}

void configureKeepAlive(FD fd, KeepAliveTimeInS time, KeepAliveIntervalInS interval, KeepAliveProbes probes)
{
    SETSOCKOPT_SOL_TCP(TCP_KEEPIDLE, time);
    SETSOCKOPT_SOL_TCP(TCP_KEEPINTVL, interval);
    SETSOCKOPT_SOL_TCP(TCP_KEEPCNT, probes);
    SETSOCKOPT_SOL(SO_KEEPALIVE, yes);
}

void configurePassCred(FD fd)
{
    SETSOCKOPT_SOL(SO_PASSCRED, yes);
}
