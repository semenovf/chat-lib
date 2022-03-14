////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.02 Initial version.
//      2022.02.17 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/conversation.hpp"
#include "pfs/chat/error.hpp"
#include "pfs/chat/backend/sqlite3/conversation.hpp"
#include <array>

namespace chat {

using namespace debby::backend::sqlite3;

namespace {
    // NOTE Must be synchronized with analog in message_store.cpp
    std::string const DEFAULT_TABLE_NAME_PREFIX { "#" };
}

namespace {

std::string const CREATE_CONVERSATION_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`message_id` {} NOT NULL UNIQUE"     // Unique message id (generated by author)
    ", `author_id` {} NOT NULL"           // Author contact ID
    ", `creation_time` {} NOT NULL"       // Creation time (UTC)
    ", `local_creation_time` {} NOT NULL" //
    ", `modification_time` {} NOT NULL"   // Modification time (UTC)
    ", `dispatched_time` {}"              // Dispatched (for outgoing) time (UTC)
    ", `delivered_time` {}"               // Delivered time (for outgoing) or received (for incoming) (UTC)
    ", `read_time` {}"                    // Read time (for outgoing and incoming) (UTC)
    ", `content` {})"                     // Message content
};

} // namespace


namespace backend {
namespace sqlite3 {

conversation::rep_type
conversation::make (contact::contact_id me
    , contact::contact_id addressee
    , shared_db_handle dbh)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.me = me;
    rep.addressee = addressee;
    rep.table_name = DEFAULT_TABLE_NAME_PREFIX + to_string(addressee);

    std::array<std::string, 1> sqls = {
        fmt::format(CREATE_CONVERSATION_TABLE
            , rep.table_name
            , affinity_traits<decltype(message::message_credentials{}.id)>::name()
            , affinity_traits<decltype(message::message_credentials{}.author_id)>::name()
            , affinity_traits<decltype(message::message_credentials{}.creation_time)>::name()
            , affinity_traits<decltype(message::message_credentials{}.local_creation_time)>::name()
            , affinity_traits<decltype(message::message_credentials{}.modification_time)>::name()
            , affinity_traits<decltype(message::message_credentials{}.dispatched_time)>::name()
            , affinity_traits<decltype(message::message_credentials{}.delivered_time)>::name()
            , affinity_traits<decltype(message::message_credentials{}.read_time)>::name()
            , affinity_traits<std::string>::name())
    };

    TRY {
        rep.dbh->begin();

        for (auto const & sql: sqls)
            rep.dbh->query(sql);

        rep.dbh->commit();
    } CATCH (debby::error ex) {
#if PFS__EXCEPTIONS_ENABLED
        rep.dbh->rollback();
        shared_db_handle empty;
        rep.dbh.swap(empty);
        throw;
#endif
    }

    return rep;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::conversation

template <>
conversation<BACKEND>::conversation (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
conversation<BACKEND>::operator bool () const noexcept
{
    return !!_rep.dbh;
}

template <>
std::size_t
conversation<BACKEND>::count () const
{
    return _rep.dbh->rows_count(_rep.table_name);
}

template <>
std::size_t
conversation<BACKEND>::unread_messages_count () const
{
    // TODO
    return 0;
}

namespace {

std::string const UPDATE_DISPATCHED_TIME {
    "UPDATE OR IGNORE `{}` SET `dispatched_time` = :time"
    " WHERE `message_id` = :message_id"
};

std::string const UPDATE_DELIVERED_TIME {
    "UPDATE OR IGNORE `{}` SET `delivered_time` = :time"
    " WHERE `message_id` = :message_id"
};

std::string const UPDATE_READ_TIME {
    "UPDATE OR IGNORE `{}` SET `read_time` = :time"
    " WHERE `message_id` = :message_id"
};

} // namespace

static void mark_message_status (
      backend::sqlite3::shared_db_handle dbh
    , std::string const & sql
    , std::string const & table_name
    , message::message_id message_id
    , pfs::utc_time_point time
    , std::string const & status_str)
{
    auto stmt = dbh->prepare(fmt::format(sql, table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":time", time);
    stmt.bind(":message_id", message_id);

    auto res = stmt.exec();

    if (stmt.rows_affected() == 0) {
        auto err = error{
              errc::message_not_found
            , fmt::format("no message mark {}: #{}"
                , status_str
                , to_string(message_id))
        };

        CHAT__THROW(err);
    }
}

template <>
void conversation<BACKEND>::mark_dispatched (message::message_id message_id
    , pfs::utc_time_point dispatched_time)
{
    mark_message_status(_rep.dbh
        , UPDATE_DISPATCHED_TIME
        , _rep.table_name
        , message_id
        , dispatched_time
        , "dispatched");
}

template <>
void conversation<BACKEND>::mark_delivered (message::message_id message_id
    , pfs::utc_time_point delivered_time)
{
    mark_message_status(_rep.dbh
        , UPDATE_DELIVERED_TIME
        , _rep.table_name
        , message_id
        , delivered_time
        , "delivered");
}

template <>
void conversation<BACKEND>::mark_read (message::message_id message_id
    , pfs::utc_time_point read_time)
{
    mark_message_status(_rep.dbh
        , UPDATE_READ_TIME
        , _rep.table_name
        , message_id
        , read_time
        , "read");
}

namespace {
std::string const INSERT_MESSAGE {
    "INSERT INTO `{}` (`message_id`, `author_id`, `creation_time`, `modification_time`, `local_creation_time`)"
    " VALUES (:message_id, :author_id, :creation_time, :modification_time, :local_creation_time)"
};
} // namespace

template <>
conversation<BACKEND>::editor_type
conversation<BACKEND>::create ()
{
    auto message_id = message::id_generator{}.next();
    auto creation_time = pfs::current_utc_time_point();

    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_MESSAGE, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":message_id", message_id);
    stmt.bind(":author_id", _rep.me);
    stmt.bind(":creation_time", creation_time);
    stmt.bind(":local_creation_time", creation_time);
    stmt.bind(":modification_time", creation_time);

    stmt.exec();

    CHAT__ASSERT(stmt.rows_affected() > 0, "Non-unique ID generated for message");

    return editor_type::make(message_id, _rep.dbh, _rep.table_name);
}

namespace {
std::string const SELECT_OUTGOING_CONTENT {
    "SELECT `message_id`"
        ", `content`"
        " FROM `{}` WHERE `message_id` = :message_id AND `author_id` = :author_id"
};              //                                                    ^
                //                                                    |
                // Interest is shown only to the outgouing message ----
} // namespace

template <>
conversation<BACKEND>::editor_type
conversation<BACKEND>::open (message::message_id message_id)
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_OUTGOING_CONTENT, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":message_id", message_id);
    stmt.bind(":author_id", _rep.me);

    auto res = stmt.exec();

    if (res.has_more()) {
        message::message_id message_id;
        pfs::optional<std::string> content_data;

        res["message_id"] >> message_id;
        res["content"]    >> content_data;

        message::content content;

        if (content_data)
            content = message::content{*content_data};

        return editor_type::make(message_id, std::move(content)
            , _rep.dbh, _rep.table_name);
    }

    return editor_type{};
}

namespace {

std::string const SELECT_MESSAGE {
    "SELECT `message_id`"
        ", `author_id`"
        ", `creation_time`"
        ", `local_creation_time`"
        ", `modification_time`"
        ", `dispatched_time`"
        ", `delivered_time`"
        ", `read_time`"
        ", `content`"
        " FROM `{}` WHERE `message_id` = :message_id"
};

} // namespace

template <>
pfs::optional<message::message_credentials>
conversation<BACKEND>::message (message::message_id message_id) const
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_MESSAGE, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":message_id", message_id);

    auto res = stmt.exec();

    if (res.has_more()) {
        std::string content_data;
        pfs::optional<message::message_credentials> result(pfs::in_place
            , message::message_credentials{});

        res["message_id"]          >> result->id;
        res["author_id"]           >> result->author_id;
        res["creation_time"]       >> result->creation_time;
        res["local_creation_time"] >> result->local_creation_time;
        res["modification_time"]   >> result->modification_time;
        res["dispatched_time"]     >> result->dispatched_time;
        res["delivered_time"]      >> result->delivered_time;
        res["read_time"]           >> result->read_time;
        res["content"]             >> content_data;

        message::content content{content_data};
        result->contents = std::move(content);
        return result;
    }

    return pfs::nullopt;
}

namespace {

std::string const SELECT_ALL_MESSAGES {
    "SELECT `message_id`"
        ", `author_id`"
        ", `creation_time`"
        ", `local_creation_time`"
        ", `modification_time`"
        ", `dispatched_time`"
        ", `delivered_time`"
        ", `read_time`"
        ", `content`"
        " FROM `{}` ORDER BY {} {}"
};

} // namespace

template <>
void
conversation<BACKEND>::for_each (std::function<void(message::message_credentials const &)> f
    , int sort_flag)
{
    std::string field = "`local_creation_time`";
    std::string order = "ASC";

    if (sort_flag & by_creation_time)
        field = "`creation_time`";
    if (sort_flag & by_local_creation_time)
        field = "`local_creation_time`";
    else if (sort_flag & by_modification_time)
        field = "`modification_time`";
    else if (sort_flag & by_dispatched_time)
        field = "`dispatched_time`";
    else if (sort_flag & by_delivered_time)
        field = "`delivered_time`";
    else if (sort_flag & by_read_time)
        field = "`read_time`";

    if (sort_flag & ascending_order)
        order = "ASC";
    else if (sort_flag & descending_order)
        order = "DESC";

    auto stmt = _rep.dbh->prepare(
        fmt::format(SELECT_ALL_MESSAGES, _rep.table_name, field, order));

    CHAT__ASSERT(!!stmt, "");

    auto res = stmt.exec();

    while (res.has_more()) {
        pfs::optional<std::string> content_data;
        message::message_credentials m;

        res["message_id"]          >> m.id;
        res["author_id"]           >> m.author_id;
        res["creation_time"]       >> m.creation_time;
        res["local_creation_time"] >> m.creation_time;
        res["modification_time"]   >> m.modification_time;
        res["dispatched_time"]     >> m.dispatched_time;
        res["delivered_time"]      >> m.delivered_time;
        res["read_time"]           >> m.read_time;
        res["content"]             >> content_data;

        if (content_data) {
            message::content content{*content_data};
            m.contents = std::move(content);
        }

        f(m);

        res.next();
    }
}

////////////////////////////////////////////////////////////////////////////////
// conversation::wipe
////////////////////////////////////////////////////////////////////////////////
template <>
void
conversation<BACKEND>::wipe ()
{
    _rep.dbh->remove(_rep.table_name);
}

} // namespace chat
