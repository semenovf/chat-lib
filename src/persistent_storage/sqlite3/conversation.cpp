////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.02 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/persistent_storage/sqlite3/conversation.hpp"
#include "pfs/chat/persistent_storage/sqlite3/transaction.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <array>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;

namespace {
    std::string const DEFAULT_TABLE_NAME_PREFIX { "#" };

    std::string const OPEN_CONVERSATION_ERROR { "open converstion table failure: {}: {}" };
    std::string const CREATE_MESSAGE_ERROR    { "create message failure: {}" };
    std::string const WIPE_ERROR              { "wipe converstion failure: {}: {}" };
    std::string const WIPE_ALL_ERROR          { "wipe all converstions failure: {}" };
}

namespace {

std::string const CREATE_CONVERSATION_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`message_id` {} NOT NULL UNIQUE"   // Unique message id (generated by author)
    ", `contact_id` {} NOT NULL"        // Author (for outgoing) or addressee (for incoming) contact ID
    ", `creation_time` {} NOT NULL"     // Creation (for outgoing) time (UTC)
    ", `dispatched_time` {}"            // Dispatched (for outgoing) time (UTC)
    ", `dlvrcv_time` {}"                // Delivered time (for outgoing) or received (for incoming) (UTC)
    ", `read_time` {}"                  // Read time (for outgoing and incoming) (UTC)
    ", `content` {})"                   // Message content
};

} // namespace

conversation::conversation (contact::contact_id c
    , database_handle_t dbh
    , failure_handler_t f)
    : base_class()
    , _contact_id(c)
    , _dbh(dbh)
    , _on_failure(f)
{
    _table_name = DEFAULT_TABLE_NAME_PREFIX + to_string(c);

    std::array<std::string, 1> sqls = {
        fmt::format(CREATE_CONVERSATION_TABLE
            , _table_name
            , affinity_traits<decltype(message::outgoing_credentials{}.id)>::name()
            , affinity_traits<decltype(message::outgoing_credentials{}.addressee_id)>::name()
            , affinity_traits<decltype(message::outgoing_credentials{}.creation_time)>::name()
            , affinity_traits<decltype(message::outgoing_credentials{}.dispatched_time)>::name()
            , affinity_traits<decltype(message::outgoing_credentials{}.delivered_time)>::name()
            , affinity_traits<decltype(message::outgoing_credentials{}.read_time)>::name()
            , affinity_traits<std::string>::name())
    };

    process_transaction(_dbh, sqls.begin(), sqls.end()
        , [this] (std::string const & sql, debby::error * perr) {
            return _dbh->query(sql, perr);
        }
        , [this] (debby::error * perr) {
            _on_failure(fmt::format(OPEN_CONVERSATION_ERROR
                , to_string(_contact_id), perr->what()));
            database_handle_t empty;
            _dbh.swap(empty);
        });
}

auto conversation::wipe_impl () -> bool
{
    debby::error err;

    if (!_dbh->remove(_table_name, & err)) {
        _on_failure(fmt::format(WIPE_ERROR, to_string(_contact_id), err.what()));
        return false;
    }

    return true;
}

auto conversation::wipe_all (database_handle_t dbh, failure_handler_t & on_failure) -> bool
{
    debby::error err;
    auto tables = dbh->tables("^" + DEFAULT_TABLE_NAME_PREFIX, & err);

    auto success = !err;

    if (success) {
        if (tables.empty())
            return true;

        success = dbh->remove(tables, & err);
    }

    if (!success)
        on_failure(fmt::format(WIPE_ALL_ERROR, err.what()));

    return success;
}

namespace {
std::string const INSERT_MESSAGE {
    "INSERT INTO `{}` (`message_id`, `contact_id`, `time1`)"
    " VALUES (:id, :addressee_id, :creation_time)"
};
} // namespace

auto conversation::create_impl (contact::contact_id addressee_id) -> editor
{
    message::outgoing_credentials m;

    m.id = message::id_generator{}.next();
    m.addressee_id = addressee_id;
    m.creation_time = pfs::current_utc_time_point();

    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(INSERT_MESSAGE, _table_name), true, & err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":id", to_storage(m.id), false, & err)
        && stmt.bind(":addressee_id", to_storage(m.addressee_id), false, & err)
        && stmt.bind(":creation_time", to_storage(m.creation_time), & err);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.is_error())
            success = false;
    }

    if (!success)
        _on_failure(fmt::format(CREATE_MESSAGE_ERROR, err.what()));
    else
        PFS__ASSERT(stmt.rows_affected() > 0, "Non-unique ID generated for message");

    return success ? editor{std::move(m)} : editor{};
}

auto conversation::count_impl () const -> std::size_t
{
    return _dbh->rows_count(_table_name);
}

}}} // namespace chat::persistent_storage::sqlite3

