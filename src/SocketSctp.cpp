#include "SocketSctp.hpp"
#include <tools/RangeStlAlgorithms.hpp>
#include "NetworkConfiguration.hpp"
#include "SctpGetAddrs.hpp"
#include "SocketConfiguration.hpp"
#include "SocketErrorChecks.hpp"
#include "SocketIO.hpp"

using namespace std;

SocketSctp::SocketSctp(const NetworkConfiguration& cfg, TaskScheduler& ts)
    : Socket(cfg.locals, ts, SOCK_SEQPACKET), config(cfg)
{
    SocketSctp::configure(fd);
    DEBUG_LOG << "Configured fd = " << fd;
    SocketSctp::bind(fd, config.locals);
}

void SocketSctp::connect(const RemoteIPSockets& remotes)
{
    for (const auto& remote : remotes)
        Socket::connect(fd, remote);
}

void SocketSctp::connectMultihomed(const RemoteIPSockets& remotes)
{
    if (remotes.empty())
        return;

    auto saddrs = toSockaddrs(remotes);
    INFO_LOG << "Connecting to " << remotes << " with multihoming";
    checkConnect(sctp_connectx(fd, asSockaddrPtr(saddrs.data()), remotes.size(), ignore_assoc));
}

void SocketSctp::send(const ChatMessage& msg)
{
    LOCK_MTX(peers_mtx);

    for (auto& p : peers)
    {
        const FD fd = p.second.fd;
        config.big_msg_size ? sendBigMessage(fd) : send(fd, msg);
    }
}

void SocketSctp::configure(FD fd)
{
    Socket::configure(fd);
    configureNoDelay(fd);
    configureReceivingEvents(fd);
    configureInitParams(fd, InitMaxAttempts{5}, InitMaxTimeoutInMs{5000});
    configureRto(fd, RtoInitialInMs{1000}, RtoMinInMs{1000}, RtoMaxInMs{5000});
    configureSack(fd, SackFreq{2}, SackDelay{200});
    configureMaxRetrans(fd, 5);
    configureIPv4AddressMapping(fd);
    configurePeerAddressParams(fd, HbInterval{10000}, PathMaxRetrans{3}, PathMtu{config.path_mtu});
    configureMaxSeg(fd, config.max_seg);
}

static void checkPorts(const LocalIPSockets& locals)
{
    set<Port> ports;
    for (const auto& local : locals)
        ports.insert(local.port);
    if (ports.size() > 1)
        throw runtime_error{"More than one port supplied for binding - all ports must be the same!"};
}

void SocketSctp::bind(FD fd, const LocalIPSockets& locals)
{
    checkPorts(locals);
    INFO_LOG << "Binding socket: fd = " << fd << ", addrs = " << locals << ", size = " << locals.size();
    auto saddrs = toSockaddrs(locals);
    checkBind(sctp_bindx(fd, asSockaddrPtr(saddrs.data()), locals.size(), SCTP_BINDX_ADD_ADDR));
    DEBUG_LOG << "Bound addresses: " << getBoundAddresses(fd);
}

LocalIPSockets SocketSctp::getBoundAddresses(FD fd) const
{
    return getLaddrs(fd);
}

RemoteIPSockets SocketSctp::getPeerAddresses(FD fd) const
{
    return getPaddrs(fd);
}

FDs SocketSctp::selectFds()
{
    LOCK_MTX(peers_mtx);
    fd_set.reset();
    fd_set.set(fd);
    for (const auto& peer : peers)
        fd_set.set(peer.second.fd);
    return fd_set.select();
}

void SocketSctp::handleMessage(FD fd)
{
    sockaddr_storage from_storage{};
    socklen_t from_len = sizeof(from_storage);
    sctp_sndrcvinfo sndrcvinfo;
    int flags = 0; // !!!

    auto& msg_buffer = getBuffer(fd);
    msg_buffer[checkedReceive(sctp_recvmsg(
        fd, msg_buffer.data(), msg_buffer.size(), asSockaddrPtr(from_storage), &from_len, &sndrcvinfo, &flags))] = 0;

    if (flags & MSG_NOTIFICATION)
        handle({from_storage, reinterpret_cast<const sctp_notification&>(msg_buffer.front())});
    else
    {
        const ChatMessage msg{msg_buffer.data()};
        INFO_LOG << "Received message: " << (msg.size() < 100 ? msg : "BIG") << " (size = " << msg.size() << ") from "
                 << from_storage;
    }
}

IOBuffer& SocketSctp::getBuffer(FD fd)
{
    LOCK_MTX(peers_mtx);
    return *(fd == this->fd ? main_msg_buffer :
                              find_if(peers, [fd](auto& peer) { return peer.second.fd == fd; })->second.msg_buffer);
}

void SocketSctp::sendBigMessage(FD fd)
{
    ChatMessage big_msg;
    big_msg.resize(config.big_msg_size);
    fill(big_msg, 'x');
    send(fd, big_msg);
}

void SocketSctp::send(FD fd, const ChatMessage& p_msg)
{
    const string msg = "HABBA";
    INFO_LOG << "Sending message: " << (p_msg.size() < 100 ? p_msg : msg) << " (size = " << p_msg.size()
             << ") on fd = " << fd;
    checkSend(::send(fd, p_msg.data(), p_msg.size(), ignore_flags));
}

namespace
{
    auto isAssocChange(const sctp_notification& sn) { return sn.sn_header.sn_type == SCTP_ASSOC_CHANGE; }

    void logPeerInfo(FD fd, AssocId assoc_id)
    {
        sctp_status status{};
        getsockopt(fd, IPPROTO_SCTP, SCTP_STATUS, status);
        const auto& paddr_info = status.sstat_primary;

        sctp_paddrparams paddr_params{};
        optInfo(fd, SCTP_PEER_ADDR_PARAMS, paddr_params);

        DEBUG_LOG << "Peeled off fd = " << fd << ", assoc_id = " << assoc_id << ", peer addresses = " << getPaddrs(fd)
                  << ", state = " << toString(static_cast<sctp_spinfo_state>(paddr_info.spinfo_state))
                  << ", cwnd = " << paddr_info.spinfo_cwnd << ", srtt = " << paddr_info.spinfo_srtt
                  << ", rto = " << paddr_info.spinfo_rto << ", mtu = " << paddr_info.spinfo_mtu
                  << ", fragmentation_point = " << status.sstat_fragmentation_point
                  << ", spp_pathmtu = " << paddr_params.spp_pathmtu;
    }
} // namespace

auto& operator<<(ostream& os, const sctp_notification& sn)
{
    switch (sn.sn_header.sn_type)
    {
        case SCTP_ASSOC_CHANGE:
        {
            switch (sn.sn_assoc_change.sac_state)
            {
                case SCTP_COMM_UP: return os << "SCTP_ASSOC_CHANGE - SCTP_COMM_UP";
                case SCTP_COMM_LOST: return os << "SCTP_ASSOC_CHANGE - SCTP_COMM_LOST";
                case SCTP_RESTART: return os << "SCTP_ASSOC_CHANGE - SCTP_RESTART";
                case SCTP_SHUTDOWN_COMP: return os << "SCTP_ASSOC_CHANGE - SCTP_SHUTDOWN_COMP";
                case SCTP_CANT_STR_ASSOC: return os << "SCTP_ASSOC_CHANGE - SCTP_CANT_STR_ASSOC";
                default: return os << "SCTP_ASSOC_CHANGE - UNKNOWN";
            }
        }
        case SCTP_PEER_ADDR_CHANGE:
        {
            const auto& msg = sn.sn_paddr_change;
            os << "SCTP_PEER_ADDR_CHANGE: assoc_id = " << msg.spc_assoc_id << ", state = ";
            switch (msg.spc_state)
            {
                case SCTP_ADDR_AVAILABLE: os << "SCTP_ADDR_AVAILABLE"; break;
                case SCTP_ADDR_UNREACHABLE: os << "SCTP_ADDR_UNREACHABLE"; break;
                case SCTP_ADDR_REMOVED: os << "SCTP_ADDR_REMOVED"; break;
                case SCTP_ADDR_ADDED: os << "SCTP_ADDR_ADDED"; break;
                case SCTP_ADDR_MADE_PRIM: os << "SCTP_ADDR_MADE_PRIM"; break;
                case SCTP_ADDR_CONFIRMED: os << "SCTP_ADDR_CONFIRMED"; break;
                default: os << "UNKNOWN";
            }
            return os << ", addr = " << IPSocket{msg.spc_aaddr};
        }
        case SCTP_REMOTE_ERROR:
            return os << "SCTP_REMOTE_ERROR";
            // case SCTP_SEND_FAILED_EVENT: return os << "SCTP_SEND_FAILED_EVENT";
        case SCTP_SHUTDOWN_EVENT: return os << "SCTP_SHUTDOWN_EVENT";
        case SCTP_ADAPTATION_INDICATION: return os << "SCTP_ADAPTATION_INDICATION";
        case SCTP_PARTIAL_DELIVERY_EVENT:
            return os << "SCTP_PARTIAL_DELIVERY_EVENT";
            // case SCTP_AUTHENTICATION_EVENT: return os << "SCTP_AUTHENTICATION_EVENT";
        case SCTP_SENDER_DRY_EVENT: return os << "SCTP_SENDER_DRY_EVENT";
        default: return os << "UNKNOWN SCTP NOTIFICATION";
    }
}

void SocketSctp::handle(const SocketSctp::Notification& n)
{
    const auto& from = n.from;
    const auto& sn = n.sn;

    INFO_LOG << "Received notification: " << sn << " from " << from;

    if (isAssocChange(sn))
    {
        const auto& sac = sn.sn_assoc_change;
        const auto state = sac.sac_state;
        const auto assoc_id = sac.sac_assoc_id;

        if (state == SCTP_COMM_UP)
        {
            DEBUG_LOG << "COMM_UP";
        }
        else
        {
            DEBUG_LOG << "Not a COMM_UP";
        }

        switch (state)
        {
            case SCTP_COMM_UP: return handleCommUp(assoc_id);
            case SCTP_CANT_STR_ASSOC: return handleEstablishmentFailure(from);
            case SCTP_SHUTDOWN_COMP: return handleGracefulShutdown(assoc_id);
            case SCTP_COMM_LOST: return handleCommLost(assoc_id, from);
        }
    }
}

void SocketSctp::handleCommUp(AssocId assoc_id)
{
    peelOff(assoc_id);
}

void SocketSctp::handleEstablishmentFailure(const RemoteIPSocket& remote)
{
    scheduleReestablishment(remote);
}

void SocketSctp::handleGracefulShutdown(AssocId assoc_id)
{
    remove(assoc_id);
}

void SocketSctp::handleCommLost(AssocId assoc_id, const RemoteIPSocket& remote)
{
    remove(assoc_id);
    scheduleReestablishment(remote);
}

void SocketSctp::peelOff(AssocId assoc_id)
{
    LOCK_MTX(peers_mtx);
    FileDescriptor peeled_fd{checkedPeeloff(sctp_peeloff(fd, assoc_id))};
    logPeerInfo(peeled_fd, assoc_id);
    peers.emplace(assoc_id, Peer{move(peeled_fd), peer_msg_buffers.get()});
}

void SocketSctp::remove(AssocId assoc_id)
{
    LOCK_MTX(peers_mtx);
    peers.erase(assoc_id);
    DEBUG_LOG << "Removed assoc_id = " << assoc_id;
}
