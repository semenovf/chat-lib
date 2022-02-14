////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2021.12.30 Refactored.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/persistent_storage/sqlite3/message_store.hpp"

using message_store_t    = chat::persistent_storage::sqlite3::message_store;
using conversation_t     = chat::persistent_storage::sqlite3::conversation;
using message_t          = chat::message::message_credentials;

auto on_failure = [] (std::string const & errstr) {
    fmt::print(stderr, "ERROR: {}\n", errstr);
};

auto message_db_path = pfs::filesystem::temp_directory_path() / "messages.db";

TEST_CASE("constructors") {
    // Conversation public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<conversation_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<conversation_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<conversation_t>::value);
    REQUIRE(std::is_move_constructible<conversation_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<conversation_t>::value);
    REQUIRE(std::is_destructible<conversation_t>::value);

    // Message store public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<message_store_t>::value);
    REQUIRE_FALSE(std::is_move_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<message_store_t>::value);
    REQUIRE(std::is_destructible<message_store_t>::value);
}

TEST_CASE("initialization") {
    auto dbh = chat::persistent_storage::sqlite3::make_handle(message_db_path, true);

    REQUIRE(dbh);

    message_store_t message_store{dbh};

    REQUIRE(message_store);
    REQUIRE(message_store.wipe());
}

TEST_CASE("outgoing messages") {
    auto dbh = chat::persistent_storage::sqlite3::make_handle(message_db_path, true);

    REQUIRE(dbh);

    message_store_t message_store{dbh};

    REQUIRE(message_store);

    message_store.wipe();

    auto my_id = chat::contact::id_generator{}.next();
    auto addressee_id = chat::contact::id_generator{}.next();
    auto conversation = message_store.conversation(my_id, addressee_id);

    for (int i = 0; i < 5; i++) {
        auto ed = conversation.create();
        REQUIRE(ed);

        ed.add_text("Hello");
        ed.add_text(", World!");
        ed.add_emoji("emoticon");
//         CHECK(ed.attach(pfs::filesystem::path{"data/attachment1.bin"}));
//         CHECK(ed.attach(pfs::filesystem::path{"data/attachment2.bin"}));
//         CHECK(ed.attach(pfs::filesystem::path{"data/attachment3.bin"}));
    }

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
