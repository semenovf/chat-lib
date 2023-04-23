////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.16 Replaced with backend declaration.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/exports.hpp"
#include <map>
#include <string>
#include <vector>

namespace chat {
namespace backend {
namespace sqlite3 {

struct contact_list
{
    struct in_memory_cache
    {
        int offset;
        int limit;
        //int sort_flags;
        std::vector<contact::contact> data;
        std::map<contact::id, std::size_t> map;
    };

    struct rep_type
    {
        shared_db_handle dbh;
        mutable in_memory_cache cache;
        std::string table_name;
    };

    /**
     * @throw @c chat::error(errc::storage_error) on storage failure.
     */
    static CHAT__EXPORT void prefetch (rep_type const * rep, int offset, int limit/*, int sort_flags*/);

    /**
     */
    static CHAT__EXPORT rep_type make (shared_db_handle dbh, std::string const & table_name);
};

}}} // namespace chat::backend::sqlite3
