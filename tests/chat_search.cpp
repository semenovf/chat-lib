////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/log.hpp"
#include "pfs/chat/chat.hpp"
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/search.hpp"
#include "pfs/chat/sqlite3.hpp"

using message_store_t = chat::message_store<chat::storage::sqlite3>;
using chat_t   = message_store_t::chat_type;
using editor_t = chat_t::editor_type;

auto message_db_path = pfs::filesystem::temp_directory_path()
    / PFS__LITERAL_PATH("conversation_search_test.db");

static char const * TAG = "CHAT";
static auto my_id = "01FV1KFY7WCBKDQZ5B4T5ZJMSA"_uuid;
static auto chat_id = "01FV1KFY7WWS3WSBV4BFYF7ZC9"_uuid;

static std::vector<chat::message::content_credentials> test_data = {
    chat::message::content_credentials {
          false
        , mime::mime_enum::text__plain
        , "Лорем ипсум долор сит амет. Вис лорем. Хис ан ЛоРеМ, куад алтера лореМ. Еи хас ЛОРЕМ."
    }
    , chat::message::content_credentials {
          false
        , mime::mime_enum::text__html
        , "<span>Лорем ипсум долор сит амет.</span> Вис лорем. Хис ан <div ЛоРеМ>ЛоРеМ</div>, куад алтера лореМ. Еи хас ЛОРЕМ."
    }
};

REGISTER_EXCEPTION_TRANSLATOR (chat::error & ex)
{
    static std::string __s {};
    __s = ex.what();
    return doctest::String(__s.c_str(), static_cast<unsigned int>(__s.size()));
}

TEST_CASE("initialization") {
    if (pfs::filesystem::exists(message_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(message_db_path) > 0);
    }

    auto db = debby::sqlite3::make(message_db_path);

    REQUIRE(db);

    auto message_store = message_store_t::make(my_id, db);

    REQUIRE(message_store);

    message_store.clear();

    auto chat = message_store.open_chat(chat_id);

    for (auto const & cc: test_data) {
        auto ed = chat.create();

        switch (cc.mime) {
            case mime::mime_enum::text__plain:
                ed.add_text(cc.text);
                ed.save();
                break;
            case mime::mime_enum::text__html:
                ed.add_html(cc.text);
                ed.save();
                break;
            default:
                break;
        }
    }
}

TEST_CASE("search") {
    auto db = debby::sqlite3::make(message_db_path);

    REQUIRE(db);

    auto message_store = message_store_t::make(my_id, db);

    REQUIRE(message_store);

    auto chat = message_store.open_chat(chat_id);
    chat::chat_searcher<decltype(chat)> chat_searcher{chat};
    auto search_result = chat_searcher.search_all("лорем"
        , chat::search_flags{chat::search_flags::ignore_case | chat::search_flags::text_content});

#if PFS__ICU_ENABLED
    REQUIRE_EQ(search_result.total_found, 10);
#else
    REQUIRE_EQ(search_result.total_found, 2);
#endif
    int counter = 1;

    for (auto const & r: search_result.m) {
        for (auto const & r1: r.m) {
            auto m = chat.message(r1.message_id);
            auto cc = m->contents->at(r1.content_index);

            std::string prefix (cc.text.begin(), cc.text.begin() + r1.m.cu_first);
            std::string substr (cc.text.begin() + r1.m.cu_first, cc.text.begin() + r1.m.cu_last);
            std::string suffix (cc.text.begin() + r1.m.cu_last, cc.text.end());

            LOGD(TAG, "{}. {}: {}[{}]{}", counter++, r1.message_id, prefix, substr, suffix);
        }
    }
}
