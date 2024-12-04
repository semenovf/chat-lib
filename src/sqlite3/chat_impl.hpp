////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.11.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "chat/sqlite3.hpp"
#include "chat/contact.hpp"
#include "chat/flags.hpp"
#include "chat/message.hpp"
#include "chat/sqlite3.hpp"
#include <atomic>
#include <map>
#include <string>

CHAT__NAMESPACE_BEGIN

namespace storage {

class sqlite3::chat
{
public:
    struct cache_data
    {
        std::atomic<bool> dirty {true};
        int offset;
        int limit;
        int sort_flags;
        std::vector<message::message_credentials> data;
        std::map<message::id, std::size_t> map;
    };

public:
    relational_database_t * pdb {nullptr};
    contact::id author_id;
    contact::id chat_id;
    mutable cache_data cache;
    std::string table_name;

public:
    chat (contact::id an_author_id, contact::id a_chat_id, relational_database_t & db);

public:
    void invalidate_cache ();
    void prefetch (int offset, int limit, int sort_flags);

public: // static
    static void fill_message (relational_database_t::result_type & result, message::message_credentials & m);
};

} // namespace storage

CHAT__NAMESPACE_END
