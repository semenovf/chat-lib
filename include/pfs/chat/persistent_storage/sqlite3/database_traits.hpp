////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/sqlite3/database.hpp"
#include "pfs/debby/sqlite3/result.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include "pfs/filesystem.hpp"
#include <memory>
#include <string>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using database_t        = debby::sqlite3::database;
using database_handle_t = std::shared_ptr<database_t>;
using result_t          = debby::sqlite3::result;
using statement_t       = debby::sqlite3::statement;
using failure_handler_t = std::function<void(std::string const &)>;

inline database_handle_t make_handle (
      pfs::filesystem::path const & path
    , bool create_if_missing = true
    , failure_handler_t on_failure = failure_handler_t{})
{
    debby::error err;

    auto dbh = std::make_shared<database_t>(path, create_if_missing, & err);

    if (err) {
        on_failure(err.what());
        dbh.reset();
    }

    return dbh;
}

}}} // namespace chat::persistent_storage::sqlite3
