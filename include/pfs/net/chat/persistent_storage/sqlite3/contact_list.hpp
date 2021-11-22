////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/net/chat/contact.hpp"
#include "pfs/emitter.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/uuid.hpp"
#include <functional>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace pfs {
namespace net {
namespace chat {
namespace sqlite3_ns {

#if PFS_HAVE_STD_FILESYSTEM
namespace filesystem = std::filesystem;
#else
namespace filesystem = pfs::filesystem;
#endif

class contact_list
{
    std::string _table_name {"contacts"};
    sqlite3 * _dbh {nullptr};
    sqlite3_stmt * _insert_contact_stmt {nullptr};

public:
    emitter_mt<std::string const &> failure;

public:
    contact_list (std::string const & table_name = "contacts")
        : _table_name(table_name)
    {}

    ~contact_list ()
    {
        if (_dbh) {
            close();
            _dbh = nullptr;
        }
    }

    contact_list (contact_list const &) = delete;
    contact_list & operator = (contact_list const &) = delete;

    contact_list (contact_list && other)
    {
        *this = std::move(other);
    }

    contact_list & operator = (contact_list && other)
    {
        other.~contact_list();
        other._dbh = _dbh;
        other._insert_contact_stmt = _insert_contact_stmt;
        _dbh = nullptr;
        _insert_contact_stmt = nullptr;
        return *this;
    }

    bool begin_transaction ();
    bool end_transaction ();
    bool rollback_transaction ();

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

    /**
     * Load contact specified by @a id from database into @a c.
     *
     * @return @c true if contact loaded successfully, @c false otherwise.
     */
    bool load (contact_id id, contact & c);

    // Wipe contacts from database
    void wipe ();

    void for_each (std::function<void(contact const &)>) const;
};

}}}} // namespace pfs::net::chat:sqlite3_ns
