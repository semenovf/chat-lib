////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define PFS_NET__P2P_TRACE_LEVEL 3
#include "pfs/net/p2p/trace.hpp"
#include "pfs/net/inet4_addr.hpp"
#include "pfs/net/p2p/engine.hpp"
#include "pfs/net/p2p/qt5/api.hpp"
#include "pfs/net/p2p/udt/api.hpp"

namespace p2p {
using inet4_addr = pfs::net::inet4_addr;

static constexpr std::uint8_t PRIORITY_COUNT = 2;

using engine = pfs::net::p2p::engine<
      pfs::net::p2p::qt5::api
    , pfs::net::p2p::udt::api
    , PRIORITY_COUNT>;
} // namespace p2p

namespace {
    const pfs::uuid_t UUID = pfs::generate_uuid();

    std::chrono::milliseconds const DISCOVERY_TRANSMIT_INTERVAL {100};
    std::chrono::milliseconds const PEER_EXPIRATION_TIMEOUT {2000};
    std::chrono::milliseconds const POLL_INTERVAL {10};
    //std::chrono::milliseconds const POLL_INTERVAL {1000};

    const p2p::inet4_addr TARGET_ADDR{227, 1, 1, 255};
    const p2p::inet4_addr DISCOVERY_ADDR{};
    const std::uint16_t DISCOVERY_PORT{4242u};
}

struct configurator
{
    p2p::inet4_addr discovery_address () const noexcept { return DISCOVERY_ADDR; }
    std::uint16_t   discovery_port () const noexcept { return DISCOVERY_PORT; }
    std::chrono::milliseconds discovery_transmit_interval () const noexcept { return DISCOVERY_TRANSMIT_INTERVAL; }
    std::chrono::milliseconds expiration_timeout () const noexcept { return PEER_EXPIRATION_TIMEOUT; }
    std::chrono::milliseconds poll_interval () const noexcept { return POLL_INTERVAL; }
    p2p::inet4_addr listener_address () const noexcept { return p2p::inet4_addr{}; }
    std::uint16_t   listener_port () const noexcept { return 4224u; }
    int backlog () const noexcept { return 10; }
};

void worker (p2p::engine & peer)
{
    while (true) {
        peer.loop();
    }
}

int main (int argc, char * argv[])
{
    std::string program{argv[0]};

    fmt::print("My name is {}\n", std::to_string(UUID));

    if (!p2p::engine::startup())
        return EXIT_FAILURE;

//     p2p::engine peer {UUID};
//
//     peer.failure.connect([] (std::string const & error) {
//         fmt::print(stderr, "!ERROR: {}\n", error);
//     });
//
//     peer.writer_ready.connect([& peer] (pfs::uuid_t uuid
//             , pfs::net::inet4_addr addr
//             , std::uint16_t port) {
//
//         TRACE_1("WRITER READY: {} ({}:{})"
//             , std::to_string(uuid)
//             , std::to_string(addr)
//             , port);
//
//         //peer.enqueue(uuid, loremipsum, std::strlen(loremipsum));
//     });
//
//     peer.rookie_accepted.connect([] (pfs::uuid_t uuid
//             , pfs::net::inet4_addr addr
//             , std::uint16_t port) {
//
//         TRACE_1("HELO: {} ({}:{})"
//             , std::to_string(uuid)
//             , std::to_string(addr)
//             , port);
//     });
//
//     peer.peer_expired.connect([] (pfs::uuid_t uuid
//             , pfs::net::inet4_addr addr
//             , std::uint16_t port) {
//
//         TRACE_1("EXPIRED: {} ({}:{})"
//             , std::to_string(uuid)
//             , std::to_string(addr)
//             , port);
//     });
//
//     peer.message_received.connect([& peer] (pfs::uuid_t uuid, std::string const & message) {
//         TRACE_1("Message received from {}: {}...{} ({}/{} characters (received/expected))"
//             , std::to_string(uuid)
//             , message.substr(0, 20)
//             , message.substr(message.size() - 20)
//             , message.size()
//             , std::strlen(loremipsum));
//
//         //peer.enqueue(uuid, loremipsum, std::strlen(loremipsum));
//     });
//
//     assert(peer.configure(configurator{}));
//
//     peer.add_discovery_target(TARGET_ADDR, DISCOVERY_PORT);
//
//     worker(peer);
//
//     p2p::engine::cleanup();

    return EXIT_SUCCESS;
}
