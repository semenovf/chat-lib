////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.06 Initial version.
//      2022.07.23 Totally refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "pfs/chat/exports.hpp"

namespace chat {
namespace backend {
namespace sqlite3 {

struct file_cache
{
    struct rep_type
    {
        shared_db_handle dbh;
        std::string out_table_name;
        std::string in_table_name;
    };

    /**
     * @throw chat::error (@c errc::filesystem_error) on filesystem error.
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    static CHAT__EXPORT rep_type make (shared_db_handle dbh);
};

}}} // namespace chat::backend::sqlite3
