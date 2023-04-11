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
#include "pfs/string_view.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/backend/sqlite3/message_store.hpp"

using message_store_t = chat::message_store<chat::backend::sqlite3::message_store>;
using conversation_t  = message_store_t::conversation_type;
using editor_t        = conversation_t::editor_type;

auto message_db_path = pfs::filesystem::temp_directory_path()
    / PFS__LITERAL_PATH("messages.db");

namespace fs = pfs::filesystem;

TEST_CASE("constructors") {
    // Conversation public constructors/assign operators
    REQUIRE(std::is_default_constructible<conversation_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<conversation_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<conversation_t>::value);
    REQUIRE(std::is_move_constructible<conversation_t>::value);
    REQUIRE(std::is_move_assignable<conversation_t>::value);
    REQUIRE(std::is_destructible<conversation_t>::value);

    // Editor public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<editor_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<editor_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<editor_t>::value);
    REQUIRE(std::is_move_constructible<editor_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<editor_t>::value);
    REQUIRE(std::is_destructible<editor_t>::value);

    // Message store public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<message_store_t>::value);
    REQUIRE(std::is_move_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<message_store_t>::value);
    REQUIRE(std::is_destructible<message_store_t>::value);
}

TEST_CASE("initialization") {
    if (pfs::filesystem::exists(message_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(message_db_path) > 0);
    }

    auto dbh = chat::backend::sqlite3::make_handle(message_db_path, true);

    REQUIRE(dbh);

    auto my_id = chat::contact::id_generator{}.next();
    auto message_store = message_store_t::make(my_id, dbh);

    REQUIRE(message_store);
    message_store.wipe();
}

TEST_CASE("outgoing messages") {
    auto dbh = chat::backend::sqlite3::make_handle(message_db_path, true);
    auto my_id = chat::contact::id_generator{}.next();
    auto message_store = message_store_t::make(my_id, dbh);
    message_store.wipe();

    auto addressee_id = chat::contact::id_generator{}.next();
    auto conversation = message_store.conversation(addressee_id);

    conversation.cache_outcome_local_file = [] (pfs::filesystem::path const & path) {
        return chat::file::file_credentials {
              chat::file::id_generator{}.next()
            , path
            , path.filename()
            , static_cast<chat::file::filesize_t>(pfs::filesystem::file_size(path))
            , utc_time_point_cast(pfs::local_time_point{pfs::filesystem::last_write_time(path).time_since_epoch()})
        };
    };

    REQUIRE(conversation);

    for (int i = 0; i < 5; i++) {
        auto ed = conversation.create();

        ed.add_text("Hello");
        ed.add_html("<html><body><h1>World</h1></body></html>");

        REQUIRE_NOTHROW(ed.attach(pfs::filesystem::path{"data/attachment1.bin"}));
        REQUIRE_NOTHROW(ed.attach(pfs::filesystem::path{"data/attachment2.bin"}));
        REQUIRE_NOTHROW(ed.attach(pfs::filesystem::path{"data/attachment3.bin"}));

        ed.save();
    }

    // Bad attachment
    {
        auto ed = conversation.create();
        REQUIRE_THROWS(ed.attach(fs::utf8_decode("ABRACADABRA")));
    }

    conversation.for_each([& conversation] (chat::message::message_credentials const & m) {
        fmt::print("{} | {} | {}\n"
            , to_string(m.message_id)
            , to_string(m.author_id)
            , to_string(m.creation_time));

        auto ed = conversation.open(m.message_id);

        if (ed.content().count() > 0) {
            REQUIRE_EQ(ed.content().at(0).mime, chat::message::mime_enum::text__plain);
            REQUIRE_EQ(ed.content().at(1).mime, chat::message::mime_enum::text__html);
            REQUIRE_EQ(ed.content().at(2).mime, chat::message::mime_enum::application__octet_stream);

            REQUIRE_EQ(ed.content().at(0).text, std::string{"Hello"});
            REQUIRE_EQ(ed.content().at(1).text, std::string{"<html><body><h1>World</h1></body></html>"});

            REQUIRE(pfs::ends_with(pfs::string_view{ed.content().at(2).text}, "attachment1.bin"));
            REQUIRE(pfs::ends_with(pfs::string_view{ed.content().at(3).text}, "attachment2.bin"));
            REQUIRE(pfs::ends_with(pfs::string_view{ed.content().at(4).text}, "attachment3.bin"));
            REQUIRE(pfs::ends_with(pfs::string_view{ed.content().attachment(2).name}, "attachment1.bin"));

            REQUIRE_EQ(ed.content().attachment(2).size, 4);

            // No attachment at specified position
            REQUIRE_EQ(ed.content().attachment(0).name, std::string{});
        }
    });
}
