////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.19 Initial version.
//      2024.11.29 Refactored for V2.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/chat/protocol.hpp"
#include "pfs/chat/primal_serializer.hpp"
#include <fstream>

namespace {

std::string TEST_CONTENT {"Lorem ipsum dolor sit amet, consectetuer adipiscing elit"};

} // namespace

TEST_CASE("serializer") {
    using serializer_t = chat::primal_serializer<pfs::endian::network>;
    auto time_point = pfs::current_utc_time_point();

    chat::protocol::regular_message m;
    m.message_id    = "01FV1KFY7WCBKDQZ5B4T5ZJMSA"_uuid;
    m.author_id     = "01FV1KFY7WWS3WSBV4BFYF7ZC9"_uuid;
    m.mod_time      = time_point;
    m.content       = TEST_CONTENT;

    serializer_t::ostream_type out;
    out << m;

    chat::protocol::regular_message m1;
    serializer_t::istream_type in {out.data(), out.size()};
    chat::protocol::packet_enum packet_type;
    in >> packet_type >> m1;

    CHECK_EQ(packet_type, chat::protocol::packet_enum::regular_message);
}
