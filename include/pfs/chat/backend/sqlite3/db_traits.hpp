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
#include "pfs/debby/relational_database.hpp"
#include "pfs/debby/result.hpp"
#include "pfs/debby/statement.hpp"
#include "pfs/debby/backend/sqlite3/database.hpp"
#include "pfs/debby/backend/sqlite3/result.hpp"
#include "pfs/debby/backend/sqlite3/statement.hpp"
#include "pfs/filesystem.hpp"
#include <memory>

namespace chat {
namespace backend {
namespace sqlite3 {

struct db_traits
{
    using database_type  = debby::relational_database<debby::backend::sqlite3::database>;
    using result_type    = debby::result<debby::backend::sqlite3::result>;
    using statement_type = debby::statement<debby::backend::sqlite3::statement>;
};

using shared_db_handle = std::shared_ptr<db_traits::database_type>;

/**
 * @throw @c chat::error on failure.
 */
shared_db_handle make_handle (pfs::filesystem::path const & path
    , bool create_if_missing = true);

}}} // namespace chat::backend::sqlite3
