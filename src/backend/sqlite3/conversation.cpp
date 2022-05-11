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

constexpr std::size_t CACHE_WINDOW_SIZE = 100;

// NOTE Must be synchronized with analog in message_store.cpp
static std::string const DEFAULT_TABLE_NAME_PREFIX { "#" };

static std::string const CREATE_CONVERSATION_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`message_id` {} NOT NULL UNIQUE"     // Unique message id (generated by author)
    ", `author_id` {} NOT NULL"           // Author contact ID
    ", `creation_time` {} NOT NULL"       // Creation time (UTC)
    ", `modification_time` {} NOT NULL"   // Modification time (UTC)
    ", `dispatched_time` {}"              // Dispatched (for outgoing) time (UTC)
    ", `delivered_time` {}"               // Delivered time (for outgoing) or received (for incoming) (UTC)
    ", `read_time` {}"                    // Read time (for outgoing and incoming) (UTC)
    ", `content` {})"                     // Message content
};

static void fill_message (backend::sqlite3::db_traits::result_type & result
    , message::message_credentials & m)
{
    pfs::optional<std::string> content_data;

    result["message_id"]        >> m.id;
    result["author_id"]         >> m.author_id;
    result["creation_time"]     >> m.creation_time;
    result["modification_time"] >> m.modification_time;
    result["dispatched_time"]   >> m.dispatched_time;
    result["delivered_time"]    >> m.delivered_time;
    result["read_time"]         >> m.read_time;
    result["content"]           >> content_data;

    if (content_data) {
        message::content content{*content_data};
        m.contents = std::move(content);
    }
}

namespace backend {
namespace sqlite3 {

void conversation::invalidate_cache (rep_type * rep)
{
    rep->cache.dirty = true;
}

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

    invalidate_cache(& rep);

    return rep;
}

static std::string const SELECT_ROWS_RANGE {
    "SELECT `message_id`"
        ", `author_id`"
        ", `creation_time`"
        ", `modification_time`"
        ", `dispatched_time`"
        ", `delivered_time`"
        ", `read_time`"
        ", `content`"
        " FROM `{}` ORDER BY {} {}"
        " LIMIT {} OFFSET {}"
};

void conversation::prefetch (rep_type const * rep
    , int offset
    , int limit
    , int sort_flag)
{
    bool prefetch_required = rep->cache.dirty
        || offset < rep->cache.offset
        || offset + limit > rep->cache.offset + rep->cache.limit
        || sort_flag != rep->cache.sort_flag;

    if (!prefetch_required)
        return;

    rep->cache.data.clear();
    rep->cache.map.clear();
    rep->cache.offset = offset;
    rep->cache.limit = 0;
    rep->cache.dirty = true;
    rep->cache.sort_flag = sort_flag;

    std::string field = "`creation_time`";
    std::string order = "ASC";

    if (sort_flag & by_creation_time)
        field = "`creation_time`";
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

    auto stmt = rep->dbh->prepare(
          fmt::format(SELECT_ROWS_RANGE, rep->table_name
        , field, order
        , limit, offset));

    CHAT__ASSERT(!!stmt, "");

    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        rep->cache.data.emplace_back();
        message::message_credentials * m = & rep->cache.data.back();
        fill_message(res, *m);
        rep->cache.map.emplace(m->id, rep->cache.data.size() - 1);
        rep->cache.limit++;
    }

    rep->cache.dirty = false;
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

static std::string const UNREAD_MESSAGES_COUNT {
    "SELECT COUNT(1) as count FROM `{}` WHERE `read_time` IS NULL"
};

template <>
std::size_t
conversation<BACKEND>::unread_messages_count () const
{
    std::size_t count = 0;
    auto stmt = _rep.dbh->prepare(fmt::format(UNREAD_MESSAGES_COUNT, _rep.table_name));
    CHAT__ASSERT(!!stmt, "");

    auto res = stmt.exec();

    if (res.has_more()) {
        count = res.get<std::size_t>(0);
        res.next();
    }

    return count;
}

static std::string const UPDATE_DISPATCHED_TIME {
    "UPDATE OR IGNORE `{}` SET `dispatched_time` = :time"
    " WHERE `message_id` = :message_id"
};

static std::string const UPDATE_DELIVERED_TIME {
    "UPDATE OR IGNORE `{}` SET `delivered_time` = :time"
    " WHERE `message_id` = :message_id"
};

static std::string const UPDATE_READ_TIME {
    "UPDATE OR IGNORE `{}` SET `read_time` = :time"
    " WHERE `message_id` = :message_id"
};

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
            , fmt::format("no message mark {}: {}"
                , status_str, message_id)
        };

        CHAT__THROW(err);
    }
}

template <>
void conversation<BACKEND>::mark_dispatched (message::message_id message_id
    , pfs::utc_time_point dispatched_time)
{
    BACKEND::invalidate_cache(& _rep);

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
    BACKEND::invalidate_cache(& _rep);

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
    BACKEND::invalidate_cache(& _rep);

    mark_message_status(_rep.dbh
        , UPDATE_READ_TIME
        , _rep.table_name
        , message_id
        , read_time
        , "read");
}

static std::string const INSERT_MESSAGE {
    "INSERT INTO `{}` (`message_id`, `author_id`, `creation_time`, `modification_time`)"
    " VALUES (:message_id, :author_id, :creation_time, :modification_time)"
};

template <>
conversation<BACKEND>::editor_type
conversation<BACKEND>::create ()
{
    BACKEND::invalidate_cache(& _rep);

    auto message_id = message::id_generator{}.next();
    auto creation_time = pfs::current_utc_time_point();

    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_MESSAGE, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":message_id", message_id);
    stmt.bind(":author_id", _rep.me);
    stmt.bind(":creation_time", creation_time);
    stmt.bind(":modification_time", creation_time);

    stmt.exec();

    CHAT__ASSERT(stmt.rows_affected() > 0, "Non-unique ID generated for message");

    return editor_type::make(message_id, _rep.dbh, _rep.table_name);
}

static std::string const SELECT_OUTGOING_CONTENT {
    "SELECT `message_id`"
        ", `content`"
        " FROM `{}` WHERE `message_id` = :message_id AND `author_id` = :author_id"
};              //                                                    ^
                //                                                    |
                // Interest is shown only to the outgouing message ----

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

////////////////////////////////////////////////////////////////////////////////
// conversation::message
////////////////////////////////////////////////////////////////////////////////
template <>
pfs::optional<message::message_credentials>
conversation<BACKEND>::message (int offset, int sort_flag) const
{
    bool force_populate_cache = _rep.cache.dirty;

    // Out of range
    if (offset < _rep.cache.offset || offset >= _rep.cache.offset + _rep.cache.limit)
        force_populate_cache = true;

    // Populate cache if dirty
    if (force_populate_cache)
        BACKEND::prefetch(& _rep, offset, CACHE_WINDOW_SIZE, sort_flag);

    if (offset < _rep.cache.offset || offset >= _rep.cache.offset + _rep.cache.limit)
        return pfs::nullopt;

    return _rep.cache.data[offset - _rep.cache.offset];
}

////////////////////////////////////////////////////////////////////////////////
// conversation::message
////////////////////////////////////////////////////////////////////////////////
static std::string const SELECT_MESSAGE {
    "SELECT `message_id`"
        ", `author_id`"
        ", `creation_time`"
        ", `modification_time`"
        ", `dispatched_time`"
        ", `delivered_time`"
        ", `read_time`"
        ", `content`"
        " FROM `{}` WHERE `message_id` = :message_id"
};

template <>
pfs::optional<message::message_credentials>
conversation<BACKEND>::message (message::message_id message_id) const
{
    // Check cache
    if (!_rep.cache.dirty) {
        auto it = _rep.cache.map.find(message_id);
        if (it != _rep.cache.map.end()) {
            assert(it->second >= 0 && it->second < _rep.cache.data.size());
            return _rep.cache.data[it->second];
        }
    }

    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_MESSAGE, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":message_id", message_id);

    auto res = stmt.exec();

    if (res.has_more()) {
        message::message_credentials m;
        fill_message(res, m);
        return m;
    }

    return pfs::nullopt;
}

static std::string const INSERT_INCOMING_MESSAGE {
    "INSERT INTO `{}` (`message_id`, `author_id`, `creation_time`"
    ", `modification_time`, `content`)"
    " VALUES (:message_id, :author_id, :creation_time"
    ", :modification_time, :content)"
};

static std::string const UPDATE_INCOMING_MESSAGE {
    "UPDATE OR IGNORE `{}` SET `creation_time` = :time"
    ", `modification_time` = :time"
    ", `content` = :content"
    " WHERE `message_id` = :message_id"
};

/**
 *
 */
template <>
void
conversation<BACKEND>::save_incoming (message::message_id message_id
    , contact::contact_id author_id
    , pfs::utc_time_point const & creation_time
    , std::string const & content)
{
    BACKEND::invalidate_cache(& _rep);

    auto m = message(message_id);

    // Message already exists
    if (m) {
        bool need_update = false;

        // Authors are different -> failure
        if (m->author_id != author_id) {
            auto err = error{
                errc::inconsistent_data
                , fmt::format("authors are different: original {} and sender {}"
                    , m->author_id, author_id)
            };

            CHAT__THROW(err);
        }

        // Content is different
        if (m->contents && !content.empty()) {
            if (m->contents->to_string() != content) {
                need_update = true;
            }
        }

        if (m->creation_time != creation_time)
            need_update = true;

        if (need_update) {
            auto stmt = _rep.dbh->prepare(fmt::format(UPDATE_INCOMING_MESSAGE, _rep.table_name));

            CHAT__ASSERT(!!stmt, "");

            stmt.bind(":time", creation_time);
            stmt.bind(":content", content);
            stmt.bind(":message_id", message_id);

            stmt.exec();
        }
    } else {
        auto stmt = _rep.dbh->prepare(fmt::format(INSERT_INCOMING_MESSAGE, _rep.table_name));

        CHAT__ASSERT(!!stmt, "");

        stmt.bind(":message_id", message_id);
        stmt.bind(":author_id", author_id);
        stmt.bind(":creation_time", creation_time);
        stmt.bind(":modification_time", creation_time);
        stmt.bind(":content", content);

        stmt.exec();

        CHAT__ASSERT(stmt.rows_affected() > 0, "May be non-unique ID for incoming message");
    }
}

////////////////////////////////////////////////////////////////////////////////
// conversation::for_each
////////////////////////////////////////////////////////////////////////////////
static std::string const SELECT_ALL_MESSAGES {
    "SELECT `message_id`"
        ", `author_id`"
        ", `creation_time`"
        ", `modification_time`"
        ", `dispatched_time`"
        ", `delivered_time`"
        ", `read_time`"
        ", `content`"
        " FROM `{}` ORDER BY {} {}"
};

template <>
void
conversation<BACKEND>::for_each (std::function<void(message::message_credentials const &)> f
    , int sort_flag)
{
    std::string field = "`creation_time`";
    std::string order = "ASC";

    if (sort_flag & by_creation_time)
        field = "`creation_time`";
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
// conversation::clear
////////////////////////////////////////////////////////////////////////////////
template <>
void
conversation<BACKEND>::clear ()
{
    BACKEND::invalidate_cache(& _rep);
    _rep.dbh->clear(_rep.table_name);
}

////////////////////////////////////////////////////////////////////////////////
// conversation::wipe
////////////////////////////////////////////////////////////////////////////////
template <>
void
conversation<BACKEND>::wipe ()
{
    BACKEND::invalidate_cache(& _rep);
    _rep.dbh->remove(_rep.table_name);
}

} // namespace chat
