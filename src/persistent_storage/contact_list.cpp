////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3_traits/contact_type_enum.hpp"
#include "sqlite3_traits/uuid.hpp"
#include "sqlite3_traits/string.hpp"
#include "sqlite3_traits/time_point.hpp"
#include "pfs/chat/persistent_storage/contact_list.hpp"
#include <cassert>

namespace pfs {
namespace chat {
namespace contact {

namespace {
    std::string const OPEN_CONTACT_LIST_ERROR { "open contact list failure: {}" };
    std::string const SAVE_CONTACT_ERROR      { "save contact failure: {}" };
    std::string const LOAD_CONTACT_ERROR      { "load contact failure: {}" };
    std::string const LOAD_ALL_CONTACTS_ERROR { "load contacts failure: {}" };
    std::string const WIPE_ERROR              { "wipe contact list failure: {}" };
} // namespace

PFS_CHAT__EXPORT contact_list::contact_list (database_handle dbh, std::string const & table_name)
    : entity_storage(dbh, table_name)
{}

namespace {

std::string const CREATE_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`id` {} NOT NULL UNIQUE"
        ", `name` {} NOT NULL"
        ", `alias` {} NOT NULL"
        ", `type` {} NOT NULL"
        ", `last_activity` {} NOT NULL"
        ", PRIMARY KEY(`id`)) WITHOUT ROWID"
};

std::string const CREATE_INDEX {
    "CREATE UNIQUE INDEX IF NOT EXISTS `contact_id` ON `{}` (`id`)"
};

} // namespace

PFS_CHAT__EXPORT bool contact_list::open_impl ()
{
    auto sql = fmt::format(CREATE_TABLE
        , _table_name
        , sqlite3::field_type<decltype(contact{}.id)>::s()
        , sqlite3::field_type<decltype(contact{}.name)>::s()
        , sqlite3::field_type<decltype(contact{}.alias)>::s()
        , sqlite3::field_type<decltype(contact{}.type)>::s()
        , sqlite3::field_type<decltype(contact{}.last_activity)>::s());

    auto success = _dbh->query(sql)
        && _dbh->query(fmt::format(CREATE_INDEX, _table_name));

    if (!success) {
        failure(fmt::format(OPEN_CONTACT_LIST_ERROR, _dbh->last_error()));
        close();
    }

    return success;
}

namespace {

std::string const INSERT_CONTACT {
    "INSERT INTO `{}` (`id`, `name`, `alias`, `type`, `last_activity`)"
    " VALUES (:id, :name, :alias, :type, :last_activity)"
};

} // namespace

PFS_CHAT__EXPORT bool contact_list::save (contact const & c)
{
    auto stmt = _dbh->prepare(fmt::format(INSERT_CONTACT, _table_name));
    bool success = !!stmt;

    success = success
        && stmt.bind(":id", sqlite3::encode(c.id))
        && stmt.bind(":name", sqlite3::encode(c.name))
        && stmt.bind(":alias", sqlite3::encode(c.alias))
        && stmt.bind(":type", sqlite3::encode(c.type))
        && stmt.bind(":last_activity", sqlite3::encode(c.last_activity));

    if (success) {
        auto res = stmt.exec();

        if (res.is_error()) {
            failure(fmt::format(SAVE_CONTACT_ERROR, res.last_error()));
            success = false;
        }
    } else {
        failure(fmt::format(SAVE_CONTACT_ERROR, stmt.last_error()));
    }

    return success;
}

namespace {

std::string const SELECT_CONTACT {
    "SELECT `id`, `name`, `alias`, `type`, `last_activity` FROM `{}` WHERE `id` = :id"
};

} // namespace

PFS_CHAT__EXPORT optional<contact> contact_list::load (contact_id id)
{
    auto stmt = _dbh->prepare(fmt::format(SELECT_CONTACT, _table_name));
    bool success = !!stmt;

    success = success && stmt.bind(":id", sqlite3::encode(id));

    if (success) {
        auto res = stmt.exec();

        if (res.has_more()) {
            contact c;
            auto failure_callback = [this] (std::string const & error) {
                failure(error);
            };

            success = sqlite3::pull(res, "id", & c.id, failure_callback)
                && sqlite3::pull(res, "name", & c.name, failure_callback)
                && sqlite3::pull(res, "alias", & c.alias, failure_callback)
                && sqlite3::pull(res, "type", & c.type, failure_callback)
                && sqlite3::pull(res, "last_activity", & c.last_activity, failure_callback);

            if (success)
                return std::move(c);
        } else {
            // Error or not found;
            success = false;

            if (res.is_error()) {
                failure(fmt::format(LOAD_CONTACT_ERROR, res.last_error()));
            }
        }
    } else {
        failure(fmt::format(LOAD_CONTACT_ERROR, stmt.last_error()));
    }

    return nullopt;
}

namespace {

std::string const WIPE_TABLE {
    "DELETE FROM `{}`"
};

} // namespace

PFS_CHAT__EXPORT void contact_list::wipe_impl ()
{
    auto success = _dbh->query(fmt::format(WIPE_TABLE, _table_name));

    if (!success)
        failure(fmt::format(WIPE_ERROR, _dbh->last_error()));
}

namespace {

std::string const SELECT_ALL_CONTACTS {
    "SELECT `id`, `name`, `alias`, `type`, `last_activity` FROM `{}`"
};

} // namespace

PFS_CHAT__EXPORT void contact_list::all_of (std::function<void(contact const &)> f)
{
    auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _table_name));
    auto success = !!stmt;

    if (success) {
        auto res = stmt.exec();

        for (; res.has_more(); res.next()) {
            contact c;

            auto failure_callback = [this] (std::string const & error) {
                failure(error);
            };

            success = sqlite3::pull(res, "id", & c.id, failure_callback)
                && sqlite3::pull(res, "name", & c.name, failure_callback)
                && sqlite3::pull(res, "alias", & c.alias, failure_callback)
                && sqlite3::pull(res, "type", & c.type, failure_callback)
                && sqlite3::pull(res, "last_activity", & c.last_activity, failure_callback);

            if (success)
                f(c);
        }

        if (res.is_error()) {
            // Error or not found;
            success = false;

            if (res.is_error()) {
                failure(fmt::format(LOAD_ALL_CONTACTS_ERROR, res.last_error()));
            }
        }
    } else {
        failure(fmt::format(LOAD_ALL_CONTACTS_ERROR, stmt.last_error()));
    }
}

}}} // namespace pfs::chat::contact
