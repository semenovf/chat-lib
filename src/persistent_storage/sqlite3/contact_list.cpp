////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "engine.hpp"
#include "pfs/net/chat/persistent_storage/sqlite3/contact_list.hpp"
#include <cassert>

namespace pfs {
namespace net {
namespace chat {
namespace sqlite3_ns {

namespace {
    std::string const CREATE_TABLE {
        "CREATE TABLE IF NOT EXISTS `{}` ("
            "`id` TEXT NOT NULL UNIQUE"
            ", `name` TEXT NOT NULL"
            ", `last_activity` TEXT NOT NULL"
            ", PRIMARY KEY(`id`))"
    };

    std::string const CREATE_INDEX {
        "CREATE UNIQUE INDEX IF NOT EXISTS `contact_id` ON `{}` (`id`)"
    };

    std::string const INSERT_CONTACT {
        "INSERT INTO `{}` (`id`, `name`, `last_activity`)"
        " VALUES (:id, :name, :last_activity)"
    };

    std::string const WIPE_TABLE {
        "DELETE FROM `{}`"
    };
}

bool contact_list::begin_transaction ()
{
    assert(_dbh);
    return ::sqlite3_ns::query(_dbh, "BEGIN TRANSACTION");
}

bool contact_list::end_transaction ()
{
    assert(_dbh);
    return ::sqlite3_ns::query(_dbh, "END TRANSACTION");
}

bool contact_list::rollback_transaction ()
{
    assert(_dbh);
    return ::sqlite3_ns::query(_dbh, "ROLLBACK TRANSACTION");
}

bool contact_list::open (filesystem::path const & path)
{
    std::string errstr;
    _dbh = ::sqlite3_ns::open(path, & errstr);

    if (_dbh) {
        auto success = ::sqlite3_ns::query(_dbh
            , fmt::format(CREATE_TABLE, _table_name)
            , & errstr);

        if (success) {
            success = ::sqlite3_ns::query(_dbh
                , fmt::format(CREATE_INDEX, _table_name)
                , & errstr);
        }

        if (!success) {
            failure(fmt::format("open contact list failure: {}", errstr));
            close();
        }
    } else {
        failure(fmt::format("open contact list failure: {}", errstr));
    }

    return _dbh != nullptr;
}

void contact_list::close ()
{
    std::string errstr;

    sqlite3_stmt * statemants[] = {_insert_contact_stmt};

    for (auto & stmt: statemants) {
        auto success = ::sqlite3_ns::finalize(stmt, & errstr);

        if (!success)
            failure(fmt::format("closing contact list error: {}", errstr));

        stmt = nullptr;
    }

    ::sqlite3_ns::close(_dbh);
    _dbh = nullptr;
}

bool contact_list::save (contact const & c)
{
    assert(_dbh);

    std::string errstr;

    bool success = true;

    if (!_insert_contact_stmt) {
        _insert_contact_stmt = ::sqlite3_ns::prepare(_dbh
            , fmt::format(INSERT_CONTACT, _table_name), & errstr);

        success = _insert_contact_stmt != nullptr;
    }

    ::sqlite3_ns::reset(_insert_contact_stmt);

    if (success) {
        success = success && ::sqlite3_ns::bind(_insert_contact_stmt
            , ":id", std::to_string(c.id), & errstr);
        success = success && ::sqlite3_ns::bind(_insert_contact_stmt
            , ":name", c.name, & errstr);
        success = success && ::sqlite3_ns::bind(_insert_contact_stmt
            , ":last_activity", c.last_activity, & errstr);

        if (success)
            success = ::sqlite3_ns::step(_insert_contact_stmt);
    }

    if (!success)
        failure(errstr);

    return success;
}

bool contact_list::load (contact_id id, contact & c)
{
    return false;
}

void contact_list::wipe ()
{
    assert(_dbh);

    std::string errstr;
    auto success = ::sqlite3_ns::query(_dbh, fmt::format(WIPE_TABLE, _table_name)
        , & errstr);

    if (!success)
        failure(errstr);
}

void contact_list::for_each (std::function<void(contact const &)>) const
{

}

}}}} // namespace pfs::net::chat::sqlite3_ns

