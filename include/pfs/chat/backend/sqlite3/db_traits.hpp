////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2022.02.16 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/error.hpp"
#include "pfs/debby/sqlite3/database.hpp"
#include "pfs/debby/sqlite3/result.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include "pfs/filesystem.hpp"
#include <memory>

namespace chat {
namespace backend {
namespace sqlite3 {

struct db_traits
{
    using database_type  = debby::sqlite3::database;
    using result_type    = debby::sqlite3::result;
    using statement_type = debby::sqlite3::statement;
};

using shared_db_handle = std::shared_ptr<db_traits::database_type>;

shared_db_handle make_handle (pfs::filesystem::path const & path
    , bool create_if_missing = true, error * perr = nullptr);

}}} // namespace chat::backend::sqlite3
