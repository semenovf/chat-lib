////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2021.12.30 Refactored.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
// #include "pfs/fmt.hpp"
// #include "pfs/string_view.hpp"
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/sqlite3.hpp"
#include <pfs/filesystem.hpp>

namespace fs = pfs::filesystem;

using message_store_t = chat::message_store<chat::storage::sqlite3>;
using chat_t = message_store_t::chat_type;
using editor_t = chat_t::editor_type;

auto message_db_path = fs::temp_directory_path() / "messages.db";

TEST_CASE("constructors") {
    // Conversation public constructors/assign operators
    REQUIRE(std::is_default_constructible<chat_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<chat_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<chat_t>::value);
    REQUIRE(std::is_move_constructible<chat_t>::value);
    REQUIRE(std::is_move_assignable<chat_t>::value);
    REQUIRE(std::is_destructible<chat_t>::value);

    // Editor public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<editor_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<editor_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<editor_t>::value);
    REQUIRE(std::is_move_constructible<editor_t>::value);
    REQUIRE(std::is_move_assignable<editor_t>::value);
    REQUIRE(std::is_destructible<editor_t>::value);

    // Message store public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<message_store_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<message_store_t>::value);
    REQUIRE(std::is_move_constructible<message_store_t>::value);
    REQUIRE(std::is_move_assignable<message_store_t>::value);
    REQUIRE(std::is_destructible<message_store_t>::value);
}

TEST_CASE("initialization") {
    if (fs::exists(message_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(message_db_path) > 0);
    }

    auto db = debby::sqlite3::make(message_db_path);

    REQUIRE(db);

    auto my_id = chat::contact::id_generator{}.next();
    auto message_store = message_store_t::make(my_id, db);

    REQUIRE(message_store);
    message_store.clear();
}

TEST_CASE("outgoing messages") {
    auto db = debby::sqlite3::make(message_db_path);
    auto my_id = chat::contact::id_generator{}.next();
    auto message_store = message_store_t::make(my_id, db);
    message_store.clear();

    auto addressee_id = chat::contact::id_generator{}.next();
    auto chat = message_store.open_chat(addressee_id);
    auto chat_id = chat.id();

    chat.cache_outgoing_local_file = [my_id, chat_id] (chat::message::id message_id
        , std::int16_t attachment_index, fs::path const & path) {
        return chat::file::credentials {my_id, chat_id, message_id, attachment_index, path};
    };

    REQUIRE(chat);

    for (int i = 0; i < 5; i++) {
        auto ed = chat.create();

        ed.add_text("Hello");
        ed.add_html("<html><body><h1>World</h1></body></html>");

        REQUIRE_NOTHROW(ed.attach(pfs::filesystem::path{"data/attachment1.bin"}));
        REQUIRE_NOTHROW(ed.attach(pfs::filesystem::path{"data/attachment2.bin"}));
        REQUIRE_NOTHROW(ed.attach(pfs::filesystem::path{"data/attachment3.bin"}));

        ed.save();
    }

    // Bad attachment
    {
        auto ed = chat.create();
        REQUIRE_THROWS(ed.attach(fs::utf8_decode("ABRACADABRA")));
    }

    chat.for_each([& chat] (chat::message::message_credentials const & m) {
        fmt::print("{} | {} | {}\n"
            , to_string(m.message_id)
            , to_string(m.author_id)
            , to_string(m.creation_time));

        auto ed = chat.open(m.message_id);

        if (ed.content().count() > 0) {
            REQUIRE_EQ(ed.content().at(0).mime, mime::mime_enum::text__plain);
            REQUIRE_EQ(ed.content().at(1).mime, mime::mime_enum::text__html);
            REQUIRE_EQ(ed.content().at(2).mime, mime::mime_enum::application__octet_stream);

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
