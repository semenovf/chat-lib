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
#include "pfs/chat/exports.hpp"
#include "pfs/chat/contact.hpp"
#include <atomic>
#include <map>
#include <string>
#include <vector>

namespace chat {
namespace backend {
namespace sqlite3 {

struct editor;

struct conversation
{
    using editor_type = editor;

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
        contact::id author_id;
        contact::id conversation_id;
        mutable in_memory_cache cache;
        std::string table_name;
        void (* invalidate_cache) (rep_type * rep);
    };

    /**
     * @throw chat::error @c errc::storage_error.
     */
    static CHAT__EXPORT rep_type make (contact::id author_id
        , contact::id conversation_id
        , shared_db_handle dbh);
};

struct editor
{
    struct rep_type
    {
        conversation::rep_type * convers;
        message::id      message_id;
        message::content content;
    };

    static CHAT__EXPORT rep_type make (conversation::rep_type * convers
        , message::id message_id);

    static CHAT__EXPORT rep_type make (conversation::rep_type * convers
        , message::id message_id
        , message::content && content);
};

}}} // namespace chat::backend::sqlite3
