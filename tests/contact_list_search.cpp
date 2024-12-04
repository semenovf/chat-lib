////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/search.hpp"
#include "pfs/chat/in_memory.hpp"
#include "pfs/chat/sqlite3.hpp"
#include <pfs/log.hpp>
#include <pfs/lorem/person.hpp>

using person_t = chat::contact::person;
using contact_manager_t = chat::contact_manager<chat::storage::sqlite3>;

auto contact_db_path = pfs::filesystem::temp_directory_path() / "contact_list_search_test.db";
auto my_uuid = pfs::generate_uuid();
auto my_alias = std::string{"My Alias"};

static char const * TAG = "CHAT";

REGISTER_EXCEPTION_TRANSLATOR (chat::error & ex)
{
    static std::string __s {};
    __s = ex.what();
    return doctest::String(__s.c_str(), static_cast<unsigned int>(__s.size()));
}

TEST_CASE("initialization") {
    if (pfs::filesystem::exists(contact_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(contact_db_path) > 0);
    }

    auto db = debby::sqlite3::make(contact_db_path);

    REQUIRE(db);

    auto contact_manager = contact_manager_t::make(chat::contact::person{my_uuid, my_alias}, db);

    REQUIRE(contact_manager);

    if (contact_manager.count() > 0)
        contact_manager.clear();

    REQUIRE_EQ(contact_manager.count(), 0);

    // Populate new data
    auto batch_add = [& contact_manager] {
        lorem::person alias_gen {lorem::lang_domain::ru_RU, lorem::gender::male};
        lorem::person desc_gen {lorem::lang_domain::ru_RU, lorem::gender::female};

        int count = 20;

        while (count-- > 0) {
            person_t c;
            c.contact_id = pfs::generate_uuid();
            c.alias = alias_gen.format(count, "%l %f");
            c.description = desc_gen.format(count, "%l %f %m");

            LOGD(TAG, "{}: {}: {}", c.contact_id, c.alias, c.description);

            contact_manager.add(std::move(c));
        }

        return pfs::optional<std::string>{};
    };

    REQUIRE_EQ(contact_manager.transaction(batch_add), pfs::nullopt);
}

TEST_CASE("search") {
    auto db = debby::sqlite3::make(contact_db_path);

    REQUIRE(db);

    auto contact_manager = contact_manager_t::make(db);

    REQUIRE(contact_manager);

    auto contact_list = contact_manager.contacts<>();
    chat::contacts_searcher<decltype(contact_list)> contacts_searcher{contact_list};

    auto search_result = contacts_searcher.search_all("нов"
        , chat::search_flags {chat::search_flags::alias_field
            | chat::search_flags::desc_field | chat::search_flags::ignore_case});

    int counter = 0;

    for (auto const & r: search_result.m) {
        auto c = contact_list.get(r.contact_id);

        counter++;

        if (r.field == chat::search_flags::alias_field) {
            std::string prefix (c.alias.begin(), c.alias.begin() + r.m.cu_first);
            std::string substr (c.alias.begin() + r.m.cu_first, c.alias.begin() + r.m.cu_last);
            std::string suffix (c.alias.begin() + r.m.cu_last, c.alias.end());

            LOGD(TAG, "{}. {}: {}[{}]{}", counter, r.contact_id, prefix, substr, suffix);
        } else {
            std::string prefix (c.description.begin(), c.description.begin() + r.m.cu_first);
            std::string substr (c.description.begin() + r.m.cu_first, c.description.begin() + r.m.cu_last);
            std::string suffix (c.description.begin() + r.m.cu_last, c.description.end());

            LOGD(TAG, "{}. {}: {}[{}]{}", counter, r.contact_id, prefix, substr, suffix);
        }
    }
}

