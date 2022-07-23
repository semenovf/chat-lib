////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
//      2022.02.17 Replaced with backend declaration.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "editor.hpp"
#include "pfs/chat/editor.hpp"
#include "pfs/chat/exports.hpp"
#include "pfs/chat/contact.hpp"
#include <atomic>
#include <map>
#include <string>
#include <vector>

namespace chat {
namespace backend {
namespace sqlite3 {

struct conversation
{
    using editor_type = chat::editor<editor>;

    struct in_memory_cache
    {
        static std::atomic<bool> dirty;

        int offset;
        int limit;
        int sort_flags;
        std::vector<message::message_credentials> data;
        std::map<message::id, std::size_t> map;
    };

    struct rep_type
    {
        shared_db_handle dbh;
        contact::id me;
        contact::id opponent;
        mutable in_memory_cache cache;
        std::string table_name;
    };

    /**
     */
    static CHAT__EXPORT void invalidate_cache (rep_type * rep);

    /**
     * @throw @c chat::error(errc::storage_error) on storage failure.
     */
    static CHAT__EXPORT void prefetch (rep_type const * rep
        , int offset
        , int limit
        , int sort_flag);

    /**
     * @throw chat::error @c errc::storage_error.
     */
    static CHAT__EXPORT rep_type make (contact::id me
        , contact::id opponent
        , shared_db_handle dbh);
};

}}} // namespace chat::backend::sqlite3
