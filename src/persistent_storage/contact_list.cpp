////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3_traits/uuid.hpp"
#include "sqlite3_traits/string.hpp"
#include "sqlite3_traits/time_point.hpp"
#include "pfs/net/chat/persistent_storage/contact_list.hpp"
#include <cassert>

namespace pfs {
namespace net {
namespace chat {

namespace {
    std::string const OPEN_CONTACT_LIST_ERROR { "open contact list failure: {}" };
    std::string const SAVE_CONTACT_ERROR      { "save contact failure: {}" };
    std::string const LOAD_CONTACT_ERROR      { "load contact failure: {}" };
    std::string const LOAD_ALL_CONTACTS_ERROR { "load contacts failure: {}" };
    std::string const WIPE_CONTACT_LIST_ERROR { "wipe contact list failure: {}" };

    std::string const CREATE_TABLE {
        "CREATE TABLE IF NOT EXISTS `{}` ("
            "`id` {} NOT NULL UNIQUE"
            ", `name` {} NOT NULL"
            ", `last_activity` {} NOT NULL"
            ", PRIMARY KEY(`id`))"
    };

    std::string const CREATE_INDEX {
        "CREATE UNIQUE INDEX IF NOT EXISTS `contact_id` ON `{}` (`id`)"
    };

    std::string const INSERT_CONTACT {
        "INSERT INTO `{}` (`id`, `name`, `last_activity`)"
        " VALUES (:id, :name, :last_activity)"
    };

    std::string const SELECT_CONTACT {
        "SELECT `id`, `name`, `last_activity` FROM `{}` WHERE `id` = :id"
    };

    std::string const SELECT_ALL_CONTACTS {
        "SELECT `id`, `name`, `last_activity` FROM `{}`"
    };

    std::string const WIPE_TABLE {
        "DELETE FROM `{}`"
    };
}

PFS_CHAT__EXPORT bool contact_list::begin_transaction ()
{
    return _dbh.query("BEGIN TRANSACTION");
}

PFS_CHAT__EXPORT bool contact_list::end_transaction ()
{
    return _dbh.query("END TRANSACTION");
}

PFS_CHAT__EXPORT bool contact_list::rollback_transaction ()
{
    return _dbh.query("ROLLBACK TRANSACTION");
}

PFS_CHAT__EXPORT bool contact_list::open (filesystem::path const & path)
{
    auto success = _dbh.open(path);

    auto sql = fmt::format(CREATE_TABLE
        , _table_name
        , sqlite3::field_type<decltype(contact{}.id)>()
        , sqlite3::field_type<decltype(contact{}.name)>()
        , sqlite3::field_type<decltype(contact{}.last_activity)>());

    success = success
        && _dbh.query(sql)
        && _dbh.query(fmt::format(CREATE_INDEX, _table_name));

    if (!success) {
        failure(fmt::format(OPEN_CONTACT_LIST_ERROR, _dbh.last_error()));
        close();
    }

    return success;
}

PFS_CHAT__EXPORT void contact_list::close ()
{
    _insert_contact_stmt.clear();
    _select_contact_stmt.clear();
    _dbh.close();
}

PFS_CHAT__EXPORT bool contact_list::save (contact const & c)
{
    bool success = true;

    if (!_insert_contact_stmt) {
        _insert_contact_stmt = _dbh.prepare(
            fmt::format(INSERT_CONTACT, _table_name));
        success = !!_insert_contact_stmt;
    }

    success = success
        && _insert_contact_stmt.bind(":id", sqlite3::encode(c.id))
        && _insert_contact_stmt.bind(":name", sqlite3::encode(c.name))
        && _insert_contact_stmt.bind(":last_activity", sqlite3::encode(c.last_activity));

    if (success) {
        auto res = _insert_contact_stmt.exec();

        if (res.is_error()) {
            failure(fmt::format(SAVE_CONTACT_ERROR, res.last_error()));
            success = false;
        }
    } else {
        failure(fmt::format(SAVE_CONTACT_ERROR, _insert_contact_stmt.last_error()));
    }

    return success;
}

PFS_CHAT__EXPORT optional<contact> contact_list::load (contact_id id)
{
    bool success = true;

    if (!_select_contact_stmt) {
        _select_contact_stmt = _dbh.prepare(
            fmt::format(SELECT_CONTACT, _table_name));
        success = !!_select_contact_stmt;
    }

    success = success && _select_contact_stmt.bind(":id", std::to_string(id));

    if (success) {
        auto res = _select_contact_stmt.exec();

        if (res.has_more()) {
            contact c;
            auto failure_callback = [this] (std::string const & error) {
                failure(error);
            };

            success = sqlite3::pull(res, "id", & c.id, failure_callback)
                && sqlite3::pull(res, "name", & c.name, failure_callback)
                && sqlite3::pull(res, "last_activity", & c.last_activity, failure_callback);

            if (success)
                return optional<contact>{std::move(c)};
        } else {
            // Error or not found;
            success = false;

            if (res.is_error()) {
                failure(fmt::format(LOAD_CONTACT_ERROR, res.last_error()));
            }
        }
    } else {
        failure(fmt::format(LOAD_CONTACT_ERROR, _select_contact_stmt.last_error()));
    }

    return optional<contact>{};
}

PFS_CHAT__EXPORT void contact_list::wipe ()
{
    auto success = _dbh.query(fmt::format(WIPE_TABLE, _table_name));

    if (!success)
        failure(fmt::format(WIPE_CONTACT_LIST_ERROR, _dbh.last_error()));
}

PFS_CHAT__EXPORT void contact_list::for_each (std::function<void(contact const &)> visitor)
{
    bool success = true;

    if (!_select_all_contact_stmt) {
        _select_all_contact_stmt = _dbh.prepare(
            fmt::format(SELECT_ALL_CONTACTS, _table_name));
        success = !!_select_all_contact_stmt;
    }

    if (success) {
        auto res = _select_all_contact_stmt.exec();

        for (; res.has_more(); res.next()) {
            contact c;

            auto failure_callback = [this] (std::string const & error) {
                failure(error);
            };

            success = sqlite3::pull(res, "id", & c.id, failure_callback)
                && sqlite3::pull(res, "name", & c.name, failure_callback)
                && sqlite3::pull(res, "last_activity", & c.last_activity, failure_callback);

            if (success)
                visitor(c);
        }

        if (res.is_error()) {
            // Error or not found;
            success = false;

            if (res.is_error()) {
                failure(fmt::format(LOAD_ALL_CONTACTS_ERROR, res.last_error()));
            }
        }
    } else {
        failure(fmt::format(LOAD_ALL_CONTACTS_ERROR, _select_all_contact_stmt.last_error()));
    }
}

}}} // namespace pfs::net::chat

