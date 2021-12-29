////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/persistent_storage/sqlite3/file_cache.hpp"

using file_cache_t = pfs::chat::persistent_storage::sqlite3::file_cache;
using file_credentials_t = pfs::chat::message::file_credentials;

TEST_CASE("file_cache") {
    auto file_cache_path = pfs::filesystem::temp_directory_path() / "file_cache.db";

    auto dbh = file_cache_t::make_handle();

    REQUIRE(dbh->open(file_cache_path));

    file_cache_t file_cache {dbh, pfs::chat::message::route_enum::incoming};

    file_cache.failure.connect([] (std::string const & errstr) {
        fmt::print(stderr, "ERROR: {}\n", errstr);
    });

    REQUIRE(file_cache.open());

    file_cache.wipe();

//     pfs::chat::message::id_generator message_id_generator;
//
//     for (int i = 0; i < 1; i++) {
//         message_t m;
//         m.id = message_id_generator.next();
//         m.deleted = false;
//         m.contact_id = pfs::generate_uuid();
//         m.creation_time = pfs::current_utc_time_point();
//
//         REQUIRE(message_store.save(m));
//     }
//
//     message_store.all_of([] (message_t const & m) {
//         fmt::print("{} | {} | {}\n"
//             , std::to_string(m.id)
//             , std::to_string(m.contact_id)
//             , to_string(m.creation_time));
//
//         CHECK_FALSE(m.received_time.has_value());
//         CHECK_FALSE(m.read_time.has_value());
//     });
//
//     message_store.close();
}

