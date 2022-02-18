////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2022.02.16 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/backend/sqlite3/db_traits.hpp"

namespace chat {
namespace backend {
namespace sqlite3 {

shared_db_handle make_handle (pfs::filesystem::path const & path
    , bool create_if_missing, error * perr)
{
    debby::error storage_err;

    auto dbh = std::make_shared<db_traits::database_type>(path
        , create_if_missing, & storage_err);

    if (storage_err) {
        dbh.reset();

        error err {errc::storage_error, storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
    }

    return dbh;
}

}}} // namespace chat::backend::sqlite3

