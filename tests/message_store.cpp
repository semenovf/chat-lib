////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
// #include "pfs/uuid.hpp"
// #include "pfs/time_point.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/persistent_storage/message_store.hpp"

using message_store_t = pfs::chat::message::message_store;
using message_t       = pfs::chat::message::credentials;

TEST_CASE("message_store") {
    auto message_store_path = pfs::filesystem::temp_directory_path() / "message_store.db";

    auto dbh = message_store_t::make_handle();

    REQUIRE(dbh->open(message_store_path));

    message_store_t message_store {dbh, pfs::chat::message::route_enum::incoming};
    message_store.failure.connect([] (std::string const & errstr) {
        fmt::print(stderr, "ERROR: {}\n", errstr);
    });

    REQUIRE(message_store.open());

    message_store.wipe();

    pfs::chat::message::id_generator message_id_generator;

    for (int i = 0; i < 1; i++) {
        message_t m;
        m.id = message_id_generator.next();
        m.deleted = false;
        m.contact_id = pfs::generate_uuid();
        m.creation_time = pfs::current_utc_time_point();

        REQUIRE(message_store.save(m));
    }

    message_store.all_of([] (message_t const & m) {
        fmt::print("{} | {} | {}\n"
            , std::to_string(m.id)
            , std::to_string(m.contact_id)
            , to_string(m.creation_time));

        CHECK_FALSE(m.received_time.has_value());
        CHECK_FALSE(m.read_time.has_value());
    });

    message_store.close();
}
