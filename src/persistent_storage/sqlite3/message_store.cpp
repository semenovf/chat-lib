////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "message_store.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <cassert>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;

namespace {
    std::string const OPEN_MESSAGE_STORE_ERROR   { "open message store failure: {}" };
    std::string const SAVE_CREDENTIALS_ERROR     { "save message credentials failure: {}" };
    std::string const LOAD_CREDENTIALS_ERROR     { "load message credentials failure: {}" };
    std::string const LOAD_ALL_CREDENTIALS_ERROR { "load messages credentials failure: {}" };
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

bool message_store::open_incoming ()
{
    auto sql = fmt::format(CREATE_INCOMING_MESSAGES_TABLE
        , _table_name
        , affinity_traits<decltype(message::credentials{}.id)>::name()
        , affinity_traits<decltype(message::credentials{}.deleted)>::name()
        , affinity_traits<decltype(message::credentials{}.contact_id)>::name()
        , affinity_traits<decltype(message::credentials{}.creation_time)>::name()
        , affinity_traits<decltype(message::credentials{}.received_time)>::name()
        , affinity_traits<decltype(message::credentials{}.read_time)>::name());

    auto success = _dbh->query(sql);

    if (!success) {
        failure(fmt::format(OPEN_MESSAGE_STORE_ERROR, _dbh->last_error()));
        close();
    }

    return success;
}

bool message_store::open_outgoing ()
{
    auto sql = fmt::format(CREATE_OUTGOING_MESSAGES_TABLE
        , _table_name
        , affinity_traits<decltype(message::credentials{}.id)>::name()
        , affinity_traits<decltype(message::credentials{}.deleted)>::name()
        , affinity_traits<decltype(message::credentials{}.contact_id)>::name()
        , affinity_traits<decltype(message::credentials{}.creation_time)>::name()
        , affinity_traits<decltype(message::credentials{}.received_time)>::name()
        , affinity_traits<decltype(message::credentials{}.read_time)>::name());

    auto success = _dbh->query(sql);
    success = success && _dbh->query(fmt::format(CREATE_OUTGOING_MESSAGES_INDEX
        , _table_name, _table_name));

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

bool message_store::save (message::credentials const & m)
{
    auto stmt = _dbh->prepare(fmt::format(INSERT_CREDENTIALS, _table_name));
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
        auto res = stmt.exec();

        if (res.is_error())
            success = false;
    }

    if (!success)
        failure(fmt::format(SAVE_CREDENTIALS_ERROR, stmt.last_error()));

    return success;
}

namespace {

std::string const SELECT_CREDENTIALS {
    "SELECT `id`, `deleted`, `contact_id`, `creation_time`"
    ", `received_time`, `read_time`"
    " FROM `{}` WHERE `id` = :id"
};

} // namespace

std::vector<message::credentials> message_store::load (message::message_id id)
{
    std::vector<message::credentials> result;

    auto stmt = _dbh->prepare(fmt::format(SELECT_CREDENTIALS, _table_name));
    bool success = !!stmt;

    success = success && stmt.bind(":id", to_storage(id));

    if (success) {
        auto res = stmt.exec();

        while (success && res.has_more()) {
            message::credentials m;
            input_record in {res};

            success = in["id"]         >> m.id
                && in["deleted"]       >> m.deleted
                && in["contact_id"]    >> m.contact_id
                && in["creation_time"] >> m.creation_time
                && in["received_time"] >> m.received_time
                && in["read_time"]     >> m.read_time;

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

void message_store::wipe ()
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

void message_store::all_of (std::function<void(message::credentials const &)> f)
{
    auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CREDENTIALS, _table_name));
    bool success = !!stmt;

    if (success) {
        auto res = stmt.exec();

        for (; res.has_more(); res.next()) {
            message::credentials m;
            input_record in {res};

            success = in["id"]         >> m.id
                && in["deleted"]       >> m.deleted
                && in["contact_id"]    >> m.contact_id
                && in["creation_time"] >> m.creation_time
                && in["received_time"] >> m.received_time
                && in["read_time"]     >> m.read_time;

            if (success)
                f(m);
        }

        if (res.is_error())
            success = false;
    }

    if (!success)
        failure(fmt::format(LOAD_ALL_CREDENTIALS_ERROR, stmt.last_error()));
}

}}} // namespace chat::persistent_storage::sqlite3
