////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/message.hpp"
#include "pfs/emitter.hpp"
#include "pfs/debby/sqlite3/database.hpp"
#include "pfs/debby/sqlite3/result.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include <memory>
#include <string>

namespace pfs {
namespace chat {

template <typename Impl>
class entity_storage
{
public:
    using database_type   = debby::sqlite3::database;
    using database_handle = std::shared_ptr<database_type>;
    using result_type     = debby::sqlite3::result;
    using statement_type  = debby::sqlite3::statement;

protected:
    std::shared_ptr<database_type> _dbh;
    std::string _table_name;

protected:
    entity_storage (database_handle dbh, std::string const & table_name)
        : _dbh(dbh)
        , _table_name(table_name)
    {}

public:
    emitter_mt<std::string const &> failure;

public:
    static database_handle make_handle ()
    {
        return std::make_shared<database_type>();
    }

public:
    ~entity_storage () {}

    entity_storage (entity_storage const &) = delete;
    entity_storage & operator = (entity_storage const &) = delete;

    entity_storage (entity_storage && other) = default;
    entity_storage & operator = (entity_storage && other) = default;

    bool open ()
    {
        return static_cast<Impl *>(this)->open_impl();
    }

    void close ()
    {
        return static_cast<Impl *>(this)->close_impl();
    }

    // Wipe contacts from database
    void wipe ()
    {
        return static_cast<Impl *>(this)->wipe_impl();
    }

    template <typename ForwardIt>
    bool save_range (ForwardIt first, ForwardIt last)
    {
        bool success = _dbh->begin();

        if (success) {
            for (int i = 0; first != last; ++first, i++)
                success = static_cast<Impl *>(this)->save(*first);
        }

        if (success) {
            _dbh->commit();
        } else {
            _dbh->rollback();
        }

        return success;
    }
};

}} // namespace pfs::chat


