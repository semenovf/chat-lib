////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
// #include "pfs/chat/contact.hpp"
#include "pfs/chat/exports.hpp"
#include <string>

namespace chat {
namespace backend {
namespace sqlite3 {

struct activity_manager
{
    struct rep_type
    {
        shared_db_handle dbh;
        std::string      log_table_name;
        std::string      brief_table_name;
    };

    /**
     */
    static CHAT__EXPORT rep_type make (shared_db_handle dbh);
};

}}} // namespace chat::backend::sqlite3

