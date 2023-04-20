////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
// #include "pfs/filesystem.hpp"
#include "pfs/log.hpp"
// #include "pfs/iterator.hpp"
// #include "pfs/memory.hpp"
// #include "pfs/universal_id.hpp"
// #include "pfs/string_view.hpp"
// #include "pfs/time_point.hpp"
// #include "pfs/unicode/char.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/backend/in_memory/contact_list.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include "pfs/lorem/person.hpp"
// #include <type_traits>
// #include <algorithm>

// using contact_t = chat::contact::contact;
using person_t = chat::contact::person;
// using group_t = chat::contact::group;
using contact_manager_t = chat::contact_manager<chat::backend::sqlite3::contact_manager>;

// FIXME REMOVE
// using contact_list_t = contact_manager_t::contact_list_type;

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

    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    auto contact_manager = contact_manager_t::make(
        chat::contact::person{my_uuid, my_alias}, dbh);

    REQUIRE(contact_manager);

    if (contact_manager.count() > 0)
        contact_manager.wipe();

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

        return true;
    };

    REQUIRE(contact_manager.transaction(batch_add));
}

TEST_CASE("search") {
    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    auto contact_manager = contact_manager_t::make(
        chat::contact::person{my_uuid, my_alias}, dbh);

    REQUIRE(contact_manager);

    auto contact_list = contact_manager.contacts<>();
    auto match = contact_list.search("нов");
}

