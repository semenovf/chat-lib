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

namespace chat = pfs::net::chat;

TEST_CASE("utc_offset") {
    {
        auto utc_offset = chat::utc_offset();
        fmt::print("UTC offset: {:+} seconds\n", utc_offset);
        fmt::print("UTC offset formatted: {}\n", chat::stringify_utc_offset(utc_offset));
    }

    {
        std:: time_t utc_offset = -1 * (30 * 60 + 12 * 3600);
        fmt::print("UTC offset: {:+} seconds\n", utc_offset);
        fmt::print("UTC offset formatted: {}\n", chat::stringify_utc_offset(utc_offset));

        CHECK(chat::stringify_utc_offset(utc_offset) == "-1230");
    }
}

TEST_CASE("time_point") {
    auto now = chat::current_time_point();
    auto millis = chat::to_millis(now);
    auto now1 = chat::from_millis(millis);
    auto millis1 = chat::to_millis(now1);

    CHECK(now == now1);
    CHECK(millis == millis1);

    fmt::print("{}\n", chat::to_iso8601(now));
}

TEST_CASE("from_iso8601") {
    auto good = chat::from_iso8601("2021-11-22T11:33:42.999+0500");
    REQUIRE(good.has_value());

    fmt::print("{}\n", chat::to_iso8601(*good));

    char const * BADS[] = {
          "202y-13-22T11:33:42.999+0500" // bad year
        , "2021-13-22T11:33:42.999+0500" // bad month
        , "2021-11-32T11:33:42.999+0500" // bad day
        , "2021-11-22T25:33:42.999+0500" // bad hour
        , "2021-11-22T11:60:42.999+0500" // bad min
        , "2021-11-22T11:33:62.999+0500" // bad sec
        , "2021-11-22T11:33:42.9w9+0500" // bad millis
        , "2021-11-22T11:33:42.999+2401" // bad timezone offset
        , "2021-11-22 11:33:42.999+2401" // bad delimiter
    };

    for (auto const & s: BADS) {
        auto bad = chat::from_iso8601(s);
        REQUIRE_FALSE(bad.has_value());
    }
}
