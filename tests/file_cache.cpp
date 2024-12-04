////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.11 Initial version.
//      2022.07.25 Refactored.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/chat/message.hpp"
#include "pfs/chat/file_cache.hpp"
#include "pfs/chat/sqlite3.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/fmt.hpp>

using file_cache_t = chat::file_cache<chat::storage::sqlite3>;
auto file_cache_db_path = pfs::filesystem::temp_directory_path()
    / PFS__LITERAL_PATH("file_cache.db");

TEST_CASE("constructors") {
    // Message store public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<file_cache_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<file_cache_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<file_cache_t>::value);
    REQUIRE(std::is_move_constructible<file_cache_t>::value);
    REQUIRE(std::is_move_assignable<file_cache_t>::value);
    REQUIRE(std::is_destructible<file_cache_t>::value);
}

TEST_CASE("file_cache") {
    if (pfs::filesystem::exists(file_cache_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(file_cache_db_path) > 0);
    }

    auto db = debby::sqlite3::make(file_cache_db_path);

    REQUIRE(db);

    auto file_cache = file_cache_t::make(db);

    REQUIRE(file_cache);
    file_cache.clear();
}
