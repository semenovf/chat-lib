////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/persistent_storage/file_cache.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <cassert>

namespace pfs {
namespace chat {
namespace message {

using namespace pfs::debby::sqlite3;

namespace {
    std::string const INCOMING_TABLE_NAME {"incoming_files"};
    std::string const OUTGOING_TABLE_NAME {"outgoing_files"};

    std::string const OPEN_ERROR             { "open file cache failure: {}" };
    std::string const SAVE_CREDENTIALS_ERROR { "save file credentials failure: {}" };
    std::string const LOAD_CREDENTIALS_ERROR { "load file credentials failure: {}" };
    std::string const LOAD_ALL_CREDENTIALS_ERROR { "load file credentials failure: {}" };
    std::string const WIPE_ERROR                 { "wipe file cache failure: {}" };
}

file_cache::file_cache (database_handle dbh, message::route_enum route)
    : file_cache(dbh
        , route
        , route == route_enum::incoming
            ? INCOMING_TABLE_NAME
            : OUTGOING_TABLE_NAME)
{}

PFS_CHAT__EXPORT file_cache::file_cache (database_handle dbh
        , message::route_enum route
        , std::string const & table_name)
    : entity_storage(dbh, table_name)
    , _route(route)
{}

namespace {

std::string const CREATE_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`message_id` {} NOT NULL"
    ", `deleted` {} NOT NULL"
    ", `name` {} NOT NULL"
    ", `size` {} NOT NULL"
    ", `sha256` {} NOT NULL)"
};

 std::string const CREATE_INDEX {
    "CREATE INDEX IF NOT EXISTS `{}_message_id}` ON `{}` (`message_id`)"
};

} // namespace

PFS_CHAT__EXPORT bool file_cache::open_helper (message::route_enum route)
{
    bool success = true;

    auto sql = fmt::format(CREATE_TABLE
        , _table_name
        , affinity_traits<decltype(file_credentials{}.msg_id)>::name()
        , affinity_traits<decltype(file_credentials{}.deleted)>::name()
        , affinity_traits<decltype(file_credentials{}.name)>::name()
        , affinity_traits<decltype(file_credentials{}.size)>::name()
        , affinity_traits<decltype(file_credentials{}.sha256)>::name());

    success = _dbh->query(sql);
    success = success &&  _dbh->query(fmt::format(CREATE_INDEX, _table_name, _table_name));

    if (!success) {
        failure(fmt::format(OPEN_ERROR, _dbh->last_error()));
        close();
    }

    return success;
}

namespace {

std::string const INSERT_CREDENTIALS {
    "INSERT INTO `{}` (`message_id`, `deleted`, `name`, `size`, `sha256`)"
    " VALUES (:message_id, :deleted, :name, :size, :sha256)"
};

} // namespace

PFS_CHAT__EXPORT bool file_cache::save (file_credentials const & f)
{
    auto stmt = _dbh->prepare(fmt::format(INSERT_CREDENTIALS, _table_name));
    bool success = !!stmt;

    success = success
        && stmt.bind(":message_id", to_storage(f.msg_id))
        && stmt.bind(":deleted", to_storage(f.deleted))
        && stmt.bind(":name", to_storage(f.name))
        && stmt.bind(":size", to_storage(f.size))
        && stmt.bind(":name", to_storage(f.sha256));

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
    "SELECT `message_id`, `deleted`, `name`, `size`, `sha256`"
    " FROM `{}` WHERE `message_id` = :message_id"
};

} // namespace

PFS_CHAT__EXPORT std::vector<file_credentials> file_cache::load (message_id msg_id)
{
    std::vector<file_credentials> result;

    auto stmt = _dbh->prepare(fmt::format(SELECT_CREDENTIALS, _table_name));
    bool success = !!stmt;

    success = success && stmt.bind(":message_id", to_storage(msg_id));

    if (success) {
        auto res = stmt.exec();

        while (success && res.has_more()) {
            file_credentials c;
            input_record in {res};

            success = in.assign("message_id").to(c.msg_id)
                && in.assign("deleted").to(c.deleted)
                && in.assign("name").to(c.name)
                && in.assign("size").to(c.size)
                && in.assign("sha256").to(c.sha256);

            if (success)
                result.push_back(std::move(c));
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

PFS_CHAT__EXPORT void file_cache::wipe_impl ()
{
    // TODO
    // Wipe files from cache directory (for incoming files)

    auto success = _dbh->query(fmt::format(WIPE_TABLE, _table_name));

    if (!success)
        failure(fmt::format(WIPE_ERROR, _dbh->last_error()));
}

namespace {

std::string const SELECT_ALL_CREDENTIALS {
    "SELECT `message_id`, `deleted`, `name`, `size`, `sha256` FROM `{}`"
};

} // namespace

PFS_CHAT__EXPORT void file_cache::all_of (std::function<void(file_credentials const &)> f)
{
    auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CREDENTIALS, _table_name));
    bool success = !!stmt;

    if (success) {
        auto res = stmt.exec();

        for (; res.has_more(); res.next()) {
            file_credentials c;
            input_record in {res};
            success = in.assign("message_id").to(c.msg_id)
                && in.assign("deleted").to(c.deleted)
                && in.assign("name").to(c.name)
                && in.assign("size").to(c.size)
                && in.assign("sha256").to(c.sha256);

            if (success)
                f(c);
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

