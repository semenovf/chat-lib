////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.13 Initial version.
//      2021.12.27 Refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/persistent_storage/sqlite3/message_store.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <cassert>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;

namespace {
    std::string const DEFAULT_INCOMING_TABLE_NAME      { "incoming_messages" };
    std::string const DEFAULT_OUTGOING_TABLE_NAME      { "outgoing_messages" };

    std::string const INCOMING { "incoming" };
    std::string const OUTGOING { "outgoing" };

    std::string const OPEN_MESSAGE_STORE_ERROR  { "open {} message store failure: {}" };
    std::string const ADD_CREDENTIALS_ERROR     { "add message credentials failure: {}" };
    std::string const LOAD_CREDENTIALS_ERROR    { "load message credentials failure: {}" };
//     std::string const LOAD_ALL_CREDENTIALS_ERROR { "load messages credentials failure: {}" };
    std::string const WIPE_ERROR                 { "wipe message store failure: {}" };
}

namespace {

std::string const CREATE_INCOMING_MESSAGES_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`id` {} NOT NULL"               // Message id (must be not unique)
    ", `deleted` {} NOT NULL"        // Deleted message flag
    ", `contact_id` {} NOT NULL"     // Author (outgoing)/ addressee (incoming) contact ID
    ", `creation_time` {} NOT NULL"  // Message creation time (UTC)
    ", `received_time` {}"           // Message received time (UTC)
    ", `read_time` {}"               // Message read time (UTC)
    ", PRIMARY KEY(`id`))"
};

std::string const CREATE_OUTGOING_MESSAGES_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`id` {} NOT NULL UNIQUE"        // Unique message id
    ", `deleted` {} NOT NULL"        // Deleted message flag
    ", `contact_id` {} NOT NULL"     // Author (outgoing)/ addressee (incoming) contact ID
    ", `creation_time` {} NOT NULL"  // Message creation time (UTC)
    ", `received_time` {}"           // Message received time (UTC)
    ", `read_time` {}"               // Message read time (UTC)
    ", PRIMARY KEY(`id`)) WITHOUT ROWID"
};

std::string const CREATE_OUTGOING_MESSAGES_INDEX {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{}_message_id` ON `{}` (`id`)"
};

} // namespace

message_store::message_store (message_store::route_enum route
    , database_handle_t dbh
    , std::function<void(std::string const &)> f)
    : base_class(f)
    , _dbh(dbh)
    , _table_name(route == route_enum::incoming
        ? DEFAULT_INCOMING_TABLE_NAME
        : DEFAULT_OUTGOING_TABLE_NAME)
{
    std::string sql;

    if (route == route_enum::incoming) {
        sql = fmt::format(CREATE_INCOMING_MESSAGES_TABLE
            , _table_name
            , affinity_traits<decltype(message::credentials{}.id)>::name()
            , affinity_traits<decltype(message::credentials{}.deleted)>::name()
            , affinity_traits<decltype(message::credentials{}.contact_id)>::name()
            , affinity_traits<decltype(message::credentials{}.creation_time)>::name()
            , affinity_traits<decltype(message::credentials{}.received_time)>::name()
            , affinity_traits<decltype(message::credentials{}.read_time)>::name());
    } else {
        sql = fmt::format(CREATE_OUTGOING_MESSAGES_TABLE
            , _table_name
            , affinity_traits<decltype(message::credentials{}.id)>::name()
            , affinity_traits<decltype(message::credentials{}.deleted)>::name()
            , affinity_traits<decltype(message::credentials{}.contact_id)>::name()
            , affinity_traits<decltype(message::credentials{}.creation_time)>::name()
            , affinity_traits<decltype(message::credentials{}.received_time)>::name()
            , affinity_traits<decltype(message::credentials{}.read_time)>::name());
    }

    debby::error err;
    auto success = _dbh->query(sql, & err);

    if (success && route == route_enum::outgoing) {
        success = _dbh->query(fmt::format(CREATE_OUTGOING_MESSAGES_INDEX
            , _table_name, _table_name), & err);
    }

    if (!success) {
        on_failure(fmt::format(OPEN_MESSAGE_STORE_ERROR
            , route == route_enum::incoming ? INCOMING : OUTGOING
            , err.what()));
        database_handle_t empty;
        _dbh.swap(empty);
    }
}

namespace {

std::string const INSERT_CREDENTIALS {
    "INSERT INTO `{}` (`id`, `deleted`, `contact_id`, `creation_time`"
    ", `received_time`, `read_time` )"
    " VALUES (:id, :deleted, :contact_id, :creation_time"
    ", :received_time, :read_time)"
};

} // namespace

int message_store::add_impl (message::credentials && m)
{
    message::credentials mm = std::move(m);
    return add_impl(mm);
}

int message_store::add_impl (message::credentials const & m)
{
    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(INSERT_CREDENTIALS, _table_name), true, & err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":id", to_storage(m.id))
        && stmt.bind(":deleted", to_storage(m.deleted))
        && stmt.bind(":contact_id", to_storage(m.contact_id))
        && stmt.bind(":creation_time", to_storage(m.creation_time));

    if (m.received_time.has_value())
        success = success && stmt.bind(":received_time", to_storage(*m.received_time));
    else
        success = success && stmt.bind(":received_time", nullptr);

    if (m.received_time.has_value())
        success = success && stmt.bind(":read_time", to_storage(*m.read_time));
    else
        success = success && stmt.bind(":read_time", nullptr);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.is_error())
            success = false;
    }

    if (!success)
        on_failure(fmt::format(ADD_CREDENTIALS_ERROR, err.what()));

    return success ? stmt.rows_affected() : -1;
}

namespace {

std::string const SELECT_CREDENTIALS {
    "SELECT `id`, `deleted`, `contact_id`, `creation_time`"
    ", `received_time`, `read_time`"
    " FROM `{}` WHERE `id` = :id"
};

} // namespace

bool message_store::fill_credentials (result_t * res, message::credentials * m)
{
    input_record in {*res};

    auto success = in["id"]    >> m->id
        && in["deleted"]       >> m->deleted
        && in["contact_id"]    >> m->contact_id
        && in["creation_time"] >> m->creation_time
        && in["received_time"] >> m->received_time
        && in["read_time"]     >> m->read_time;

    return success;
}

std::vector<message::credentials> message_store::get_impl (message::message_id id)
{
    std::vector<message::credentials> result;

    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_CREDENTIALS, _table_name), true, & err);
    bool success = !!stmt;

    success = success && stmt.bind(":id", to_storage(id), false, & err);

    if (success) {
        auto res = stmt.exec(& err);

        while (success && res.has_more()) {
            message::credentials m;
            success = fill_credentials(& res, & m);

            if (success)
                result.push_back(std::move(m));
        }
    }

    if (!success)
        failure(fmt::format(LOAD_CREDENTIALS_ERROR, err.what()));

    return result;
}

namespace {

std::string const WIPE_TABLE {
    "DELETE FROM `{}`"
};

} // namespace

bool message_store::wipe_impl ()
{
    debby::error err;
    auto success = _dbh->query(fmt::format(WIPE_TABLE, _table_name), & err);

    if (!success)
        on_failure(fmt::format(WIPE_ERROR, err.what()));

    return success;
}

// namespace {
//
// std::string const SELECT_ALL_CREDENTIALS {
//     "SELECT `id`, `deleted`, `contact_id`, `creation_time`"
//     ", `received_time` , `read_time` FROM `{}`"
// };
//
// } // namespace
//
// void message_store::all_of (std::function<void(message::credentials const &)> f)
// {
//     auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CREDENTIALS, _table_name));
//     bool success = !!stmt;
//
//     if (success) {
//         auto res = stmt.exec();
//
//         for (; res.has_more(); res.next()) {
//             message::credentials m;
//             input_record in {res};
//
//             success = in["id"]         >> m.id
//                 && in["deleted"]       >> m.deleted
//                 && in["contact_id"]    >> m.contact_id
//                 && in["creation_time"] >> m.creation_time
//                 && in["received_time"] >> m.received_time
//                 && in["read_time"]     >> m.read_time;
//
//             if (success)
//                 f(m);
//         }
//
//         if (res.is_error())
//             success = false;
//     }
//
//     if (!success)
//         failure(fmt::format(LOAD_ALL_CREDENTIALS_ERROR, stmt.last_error()));
// }

}}} // namespace chat::persistent_storage::sqlite3
