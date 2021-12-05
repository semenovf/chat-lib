////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3_traits/bool.hpp"
#include "sqlite3_traits/string.hpp"
#include "sqlite3_traits/time_point.hpp"
#include "sqlite3_traits/uuid.hpp"
#include "pfs/chat/persistent_storage/message_store.hpp"
#include <cassert>

namespace pfs {
namespace chat {
namespace message {

namespace {
    std::string const INCOMING_TABLE_NAME {"incoming_messages"};
    std::string const OUTGOING_TABLE_NAME {"outgoing_messages"};
    std::string const EMPTY_STRING {};
    std::string const UNIQUE_KEYWORD {"UNIQUE"};
    std::string const WITHOUT_ROWID_KEYWORD {"WITHOUT ROWID"};

    std::string const OPEN_MESSAGE_STORE_ERROR   { "open message store failure: {}" };
    std::string const SAVE_CREDENTIALS_ERROR     { "save message credentials failure: {}" };
    std::string const LOAD_CREDENTIALS_ERROR     { "load message credentials failure: {}" };
    std::string const LOAD_ALL_CREDENTIALS_ERROR { "load messages credentials failure: {}" };
    std::string const WIPE_ERROR                 { "wipe message store failure: {}" };
}

message_store::message_store (database_handle dbh, route_enum route)
    : message_store(dbh
        , route
        , route == route_enum::incoming
            ? INCOMING_TABLE_NAME
            : OUTGOING_TABLE_NAME)
{}

PFS_CHAT__EXPORT message_store::message_store (database_handle dbh
        , route_enum route
        , std::string const & table_name)
    : entity_storage(dbh, table_name)
    , _route(route)
{}

namespace {

std::string const CREATE_MESSAGES_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`id` {} NOT NULL {}"            // Unique message id (must be unique for outgoing messages)
    ", `deleted` {} NOT NULL"        // Deleted message flag
    ", `contact_id` {} NOT NULL"     // Author (outgoing)/ addressee (incoming) contact ID
    ", `creation_time` {} NOT NULL"  // Message creation time (UTC)
    ", `received_time` {}"           // Message received time (UTC)
    ", `read_time` {}"               // Message read time (UTC)
    ", PRIMARY KEY(`id`)) {}"        // 'WITHOUT ROWID' (optional)
};

std::string const CREATE_OUTGOING_MESSAGES_INDEX {
    "CREATE UNIQUE INDEX IF NOT EXISTS `message_id` ON `{}` (`id`)"
};

} // namespace

PFS_CHAT__EXPORT bool message_store::open_helper (route_enum route)
{
    bool success = true;

    std::array<std::string const *, 10> replacements {
          & CREATE_MESSAGES_TABLE, & EMPTY_STRING, & EMPTY_STRING, & EMPTY_STRING
        , & CREATE_MESSAGES_TABLE, & UNIQUE_KEYWORD, & CREATE_OUTGOING_MESSAGES_INDEX, & WITHOUT_ROWID_KEYWORD
    };

    int base_index = (static_cast<int>(route) - 1) * 4;

    auto sql_format           = replacements[base_index + 0];
    auto unique_keyword       = replacements[base_index + 1];
    auto sql_index_format     = replacements[base_index + 2];
    auto withot_rowid_keyword = replacements[base_index + 3];

    auto sql = fmt::format(*sql_format
        , _table_name
        , sqlite3::field_type<decltype(credentials{}.id)>::s()
        , *unique_keyword
        , sqlite3::field_type<decltype(credentials{}.deleted)>::s()
        , sqlite3::field_type<decltype(credentials{}.contact_id)>::s()
        , sqlite3::field_type<decltype(credentials{}.creation_time)>::s()
        , sqlite3::field_type<decltype(credentials{}.received_time)>::s()
        , sqlite3::field_type<decltype(credentials{}.read_time)>::s()
        , *withot_rowid_keyword);

    success = _dbh->query(sql);

    if (success && !sql_index_format->empty())
        success = _dbh->query(fmt::format(*sql_index_format, _table_name));

    if (!success) {
        failure(fmt::format(OPEN_MESSAGE_STORE_ERROR, _dbh->last_error()));
        close();
    }

    return success;
}

namespace {

std::string const INSERT_CREDENTIALS {
    "INSERT INTO `{}` (`id`, `deleted`, `contact_id`, `creation_time`"
    ", `received_time`, `read_time` )"
    " VALUES (:id, :deleted, :contact_id, :creation_time"
    ", :received_time, :read_time)"
};

} // namespace

PFS_CHAT__EXPORT bool message_store::save (message::credentials const & m)
{
    auto stmt = _dbh->prepare(fmt::format(INSERT_CREDENTIALS, _table_name));
    bool success = !!stmt;

    success = success
        && stmt.bind(":id", sqlite3::encode(m.id))
        && stmt.bind(":deleted", sqlite3::encode(m.deleted))
        && stmt.bind(":contact_id", sqlite3::encode(m.contact_id))
        && stmt.bind(":creation_time", sqlite3::encode(m.creation_time));

    if (m.received_time.has_value())
        success = success && stmt.bind(":received_time", sqlite3::encode(*m.received_time));
    else
        success = success && stmt.bind(":received_time", nullptr);

    if (m.received_time.has_value())
        success = success && stmt.bind(":read_time", sqlite3::encode(*m.read_time));
    else
        success = success && stmt.bind(":read_time", nullptr);

    if (success) {
        auto res = stmt.exec();

        if (res.is_error()) {
            failure(fmt::format(SAVE_CREDENTIALS_ERROR, res.last_error()));
            success = false;
        }
    } else {
        failure(fmt::format(SAVE_CREDENTIALS_ERROR, stmt.last_error()));
    }

    return success;
}

namespace {

std::string const SELECT_CREDENTIALS {
    "SELECT `id`, `deleted`, `contact_id`, `creation_time`"
    ", `received_time`, `read_time`"
    " FROM `{}` WHERE `id` = :id"
};

} // namespace

PFS_CHAT__EXPORT std::vector<message::credentials> message_store::load (message_id id)
{
    std::vector<message::credentials> result;

    auto stmt = _dbh->prepare(fmt::format(SELECT_CREDENTIALS, _table_name));
    bool success = !!stmt;

    success = success && stmt.bind(":id", sqlite3::encode(id));

    if (success) {
        auto res = stmt.exec();

        while (success && res.has_more()) {
            message::credentials m;

            auto failure_callback = [this] (std::string const & error) {
                failure(error);
            };

            success = sqlite3::pull(res, "id", & m.id, failure_callback)
                && sqlite3::pull(res, "deleted", & m.deleted, failure_callback)
                && sqlite3::pull(res, "contact_id", & m.contact_id, failure_callback)
                && sqlite3::pull(res, "creation_time", & m.creation_time, failure_callback)
                && sqlite3::pull(res, "received_time", & m.received_time, failure_callback)
                && sqlite3::pull(res, "read_time", & m.read_time, failure_callback);

            if (success)
                result.push_back(std::move(m));
        }
    }

    if (!success) {
        result.clear();
        failure(fmt::format(LOAD_CREDENTIALS_ERROR, stmt.last_error()));
    }

    return result;
}

namespace {

std::string const WIPE_TABLE {
    "DELETE FROM `{}`"
};

} // namespace

PFS_CHAT__EXPORT void message_store::wipe_impl ()
{
    auto success = _dbh->query(fmt::format(WIPE_TABLE, _table_name));

    if (!success)
        failure(fmt::format(WIPE_ERROR, _dbh->last_error()));
}

namespace {

std::string const SELECT_ALL_CREDENTIALS {
    "SELECT `id`, `deleted`, `contact_id`, `creation_time`"
    ", `received_time` , `read_time` FROM `{}`"
};

} // namespace

PFS_CHAT__EXPORT void message_store::all_of (std::function<void(message::credentials const &)> f)
{
    auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CREDENTIALS, _table_name));
    bool success = !!stmt;

    if (success) {
        auto res = stmt.exec();

        for (; res.has_more(); res.next()) {
            message::credentials m;

            auto failure_callback = [this] (std::string const & error) {
                failure(error);
            };

            success = sqlite3::pull(res, "id", & m.id, failure_callback)
                && sqlite3::pull(res, "deleted", & m.deleted, failure_callback)
                && sqlite3::pull(res, "contact_id", & m.contact_id, failure_callback)
                && sqlite3::pull(res, "creation_time", & m.creation_time, failure_callback)
                && sqlite3::pull(res, "received_time", & m.received_time, failure_callback)
                && sqlite3::pull(res, "read_time", & m.read_time, failure_callback);

            if (success)
                f(m);
        }

        if (res.is_error()) {
            // Error or not found;
            success = false;
        }
    }

    if (!success)
        failure(fmt::format(LOAD_ALL_CREDENTIALS_ERROR, stmt.last_error()));
}

}}} // namespace pfs::chat::message
