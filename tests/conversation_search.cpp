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
#include "pfs/chat/conversation.hpp"
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/backend/sqlite3/conversation.hpp"
#include "pfs/chat/backend/sqlite3/message_store.hpp"

using message_store_t = chat::message_store<chat::backend::sqlite3::message_store>;
using conversation_t  = message_store_t::conversation_type;
using editor_t        = conversation_t::editor_type;

auto message_db_path = pfs::filesystem::temp_directory_path()
    / PFS__LITERAL_PATH("conversation_search_test.db");

static char const * TAG = "CHAT";
static auto my_id = "01FV1KFY7WCBKDQZ5B4T5ZJMSA"_uuid;
static auto conversation_id = "01FV1KFY7WWS3WSBV4BFYF7ZC9"_uuid;

static std::vector<chat::message::content_credentials> test_data = {
    {
          chat::message::mime_enum::text__plain
        , "Лорем ипсум долор сит амет. Вис лорем. Хис ан ЛоРеМ, куад алтера лореМ. Еи хас ЛОРЕМ."
    }
    , {
          chat::message::mime_enum::text__html
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

    auto dbh = chat::backend::sqlite3::make_handle(message_db_path, true);

    REQUIRE(dbh);

    auto message_store = message_store_t::make(my_id, dbh);

    REQUIRE(message_store);
    message_store.wipe();

    auto conversation = message_store.conversation(conversation_id);

    for (auto const & cc: test_data) {
        auto ed = conversation.create();

        switch (cc.mime) {
            case chat::message::mime_enum::text__plain:
                ed.add_text(cc.text);
                ed.save();
                break;
            case chat::message::mime_enum::text__html:
                ed.add_html(cc.text);
                ed.save();
                break;
            default:
                break;
        }
    }
}

TEST_CASE("search") {
    auto dbh = chat::backend::sqlite3::make_handle(message_db_path, true);

    REQUIRE(dbh);

    auto message_store = message_store_t::make(my_id, dbh);

    REQUIRE(message_store);

    auto conversation = message_store.conversation(conversation_id);

    auto search_result = conversation.search_all("лорем"
         , conversation.ignore_case_flag | conversation.text_content_flag);

    int counter = 0;

    for (auto const & r: search_result.m) {
        auto m = conversation.message(r.message_id);

        counter++;

        auto cc = m->contents->at(r.content_index);

        std::string prefix (cc.text.begin(), cc.text.begin() + r.m.cu_first);
        std::string substr (cc.text.begin() + r.m.cu_first, cc.text.begin() + r.m.cu_last);
        std::string suffix (cc.text.begin() + r.m.cu_last, cc.text.end());

        LOGD(TAG, "{}. {}: {}[{}]{}", counter, r.message_id, prefix, substr, suffix);
    }

    REQUIRE_EQ(counter, 10);
}
