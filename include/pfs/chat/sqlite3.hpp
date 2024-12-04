////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.11.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "contact.hpp"
#include "error.hpp"
#include "exports.hpp"
#include <pfs/debby/relational_database.hpp>
#include <pfs/debby/sqlite3.hpp>
#include <cstdint>
#include <functional>
#include <string>

CHAT__NAMESPACE_BEGIN

namespace storage {

struct sqlite3
{
    using relational_database_t = debby::relational_database<debby::backend_enum::sqlite3>;

    class activity_manager;
    class contact_list;
    class contact_manager;
    class chat;
    class file_cache;
    class editor;
    class message_store;

    // Default is "#"
    static std::function<std::string ()> chat_table_name_prefix;

    // Default is 100
    static std::function<std::size_t ()> cache_window_size;

    // Default is "activity_log"
    static std::function<std::string ()> activity_log_table_name;

    // Default is "activity_brief"
    static std::function<std::string ()> activity_brief_table_name;

    // Default is "file_cache_in"
    static std::function<std::string ()> incoming_table_name;

    // Default is "file_cache_out"
    static std::function<std::string ()> outgoing_table_name;

    static CHAT__EXPORT contact_list * make_contact_list (std::string table_name
        , relational_database_t & db);

    /**
     * Create contact manager instance with initialization of self contact information.
     */
    static CHAT__EXPORT contact_manager * make_contact_manager (contact::person const & my_contact
        , relational_database_t & db);

    /**
     * Create contact manager instance.
     */
    static CHAT__EXPORT contact_manager * make_contact_manager (debby::relational_database<debby::backend_enum::sqlite3> & db);

    /**
     * Create message store instance.
     */
    static CHAT__EXPORT message_store * make_message_store (contact::id my_contact_id
        , debby::relational_database<debby::backend_enum::sqlite3> & db);

    /**
     * Create activity manager instance.
     */
    static CHAT__EXPORT activity_manager * make_activity_manager (debby::relational_database<debby::backend_enum::sqlite3> & db);

    /**
     * Create activity manager instance.
     */
    static CHAT__EXPORT file_cache * make_file_cache (debby::relational_database<debby::backend_enum::sqlite3> & db);
};

} // namespace storage

CHAT__NAMESPACE_END
