////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/net/chat/time_point.hpp"

TEST_CASE("time_point") {
    namespace chat = pfs::net::chat;

    auto now = chat::current_time_point();
    auto millis = chat::to_millis(now);
    auto now1 = chat::from_millis(millis);
    auto millis1 = chat::to_millis(now1);

    CHECK(now == now1);
    CHECK(millis == millis1);

    fmt::print("{}\n", chat::to_iso8601_string(now));
}
