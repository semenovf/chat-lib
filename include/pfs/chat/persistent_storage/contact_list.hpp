////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/contact.hpp"
#include "pfs/chat/exports.hpp"
#include "pfs/emitter.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/uuid.hpp"
#include "pfs/debby/sqlite3/database.hpp"
#include "pfs/debby/sqlite3/result.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include <functional>
#include <string>

namespace pfs {
namespace chat {

PFS_CHAT__EXPORT class contact_list
{
    using database_type  = debby::sqlite3::database;
    using result_type    = debby::sqlite3::result;
    using statement_type = debby::sqlite3::statement;

    std::string _table_name {"contacts"};
    database_type _dbh;

public:
    emitter_mt<std::string const &> failure;

public:
    contact_list (std::string const & table_name = "contacts")
        : _table_name(table_name)
    {}

    ~contact_list () = default;

    contact_list (contact_list const &) = delete;
    contact_list & operator = (contact_list const &) = delete;

    contact_list (contact_list && other) = default;
    contact_list & operator = (contact_list && other) = default;

    /**
     * Open contact list database.
     */
    bool open (filesystem::path const & path);

    /**
     * Close contact list database.
     */
    void close ();

    /**
     * Save contact into database.
     */
    bool save (contact const & c);

    template <typename ForwardIt>
    bool save (ForwardIt first, ForwardIt last)
    {
        bool success = _dbh.begin();

        if (success) {
            for (int i = 0; first != last; ++first, i++)
                success = save(*first);
                //fmt::print("[{}]: [{}]\n", i, (*first).name);
        }

        if (success) {
            _dbh.commit();
        } else {
            _dbh.rollback();
        }

        return success;
    }

    /**
     * Load contact specified by @a id from database.
     */
    optional<contact> load (contact_id id);

    // Wipe contacts from database
    void wipe ();

    void all_of (std::function<void(contact const &)>);
};

}} // namespace pfs::chat
