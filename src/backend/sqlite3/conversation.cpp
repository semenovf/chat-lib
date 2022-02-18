////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.02 Initial version.
//      2022.02.17 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat//conversation.hpp"
#include "pfs/chat/backend/sqlite3/conversation.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <array>

namespace chat {

using namespace debby::sqlite3;

namespace {
    // NOTE Must be synchronized with analog in message_store.cpp
    std::string const DEFAULT_TABLE_NAME_PREFIX { "#" };

    std::string const OPEN_CONVERSATION_ERROR { "open converstion table failure: {}: {}" };
    std::string const CREATE_MESSAGE_ERROR    { "create message failure: {}" };
    std::string const WIPE_ERROR              { "wipe converstion failure: {}: {}" };
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
    , shared_db_handle dbh
    , error * perr)
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

    debby::error storage_err;
    auto success = rep.dbh->begin();

    if (success) {
        for (auto const & sql: sqls) {
            success = success && rep.dbh->query(sql, & storage_err);
        }
    }

    if (success)
        rep.dbh->commit();
    else
        rep.dbh->rollback();

    if (!success) {
        shared_db_handle empty;
        rep.dbh.swap(empty);
        error err {errc::storage_error
            , fmt::format(OPEN_CONVERSATION_ERROR, storage_err.what())};
        if (perr) *perr = err; CHAT__THROW(err);
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
std::string const INSERT_MESSAGE {
    "INSERT INTO `{}` (`message_id`, `author_id`, `creation_time`, `modification_time`, `local_creation_time`)"
    " VALUES (:id, :author_id, :creation_time, :modification_time, :local_creation_time)"
};
} // namespace

template <>
conversation<BACKEND>::editor_type
conversation<BACKEND>::create (error * perr)
{
    auto message_id = message::id_generator{}.next();
    auto creation_time = pfs::current_utc_time_point();

    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_MESSAGE, _rep.table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":id", to_storage(message_id), false, & storage_err)
        && stmt.bind(":author_id", to_storage(_rep.me), false, & storage_err)
        && stmt.bind(":creation_time", to_storage(creation_time), & storage_err)
        && stmt.bind(":local_creation_time", to_storage(creation_time), & storage_err)
        && stmt.bind(":modification_time", to_storage(creation_time), & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error()) {
            success = false;
        }
    }

    if (!success) {
        error err {errc::storage_error
            , fmt::format(CREATE_MESSAGE_ERROR, storage_err.what())};
        if (perr) *perr = err; else CHAT__THROW(err);
    } else {
        PFS__ASSERT(stmt.rows_affected() > 0, "Non-unique ID generated for message");
    }

    return success
        ? editor_type::make(message_id, _rep.dbh, _rep.table_name)
        : editor_type{};
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
conversation<BACKEND>::open (message::message_id message_id, error * perr)
{
    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_OUTGOING_CONTENT, _rep.table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":message_id", to_storage(message_id), false, & storage_err)
        && stmt.bind(":author_id", to_storage(_rep.me), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.has_more()) {
            input_record in {res};
            message::message_id message_id;
            std::string content_data;

            success = in["message_id"] >> message_id
                && in["content"]       >> content_data;

            if (success) {
                error err;
                message::content content{content_data, & err};

                if (!content) {
                    if (perr) *perr = err; else CHAT__THROW(err);
                    return editor_type{};
                }

                return editor_type::make(message_id, std::move(content)
                    , _rep.dbh, _rep.table_name);
            }
        }

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , fmt::format("fetch outgoing message failure: #{}", to_string(message_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
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
conversation<BACKEND>::message (message::message_id message_id, error * perr) const noexcept
{
    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_MESSAGE, _rep.table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":message_id", to_storage(message_id), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.has_more()) {
            input_record in {res};
            std::string content_data;
            pfs::optional<message::message_credentials> result(pfs::in_place, message::message_credentials{});

            success = in["message_id"]       >> result->id
                && in["author_id"]           >> result->author_id
                && in["creation_time"]       >> result->creation_time
                && in["local_creation_time"] >> result->local_creation_time
                && in["modification_time"]   >> result->modification_time
                && in["dispatched_time"]     >> result->dispatched_time
                && in["delivered_time"]      >> result->delivered_time
                && in["read_time"]           >> result->read_time
                && in["content"]             >> content_data;

            if (success) {
                error err;
                message::content content{content_data, & err};

                if (!content) {
                    if (perr) *perr = err;
                    return pfs::nullopt;
                }

                result->contents = std::move(content);
                return result;
            }
        }

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , fmt::format("fetch message failure: #{}", to_string(message_id))
            , storage_err.what()};
        if (perr) *perr = err;
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
bool
conversation<BACKEND>::for_each (std::function<void(message::message_credentials const &)> f
    , int sort_flag, error * perr)
{
    debby::error storage_err;

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
          fmt::format(SELECT_ALL_MESSAGES, _rep.table_name, field, order)
        , true, & storage_err);
    bool success = !!stmt;

    if (success) {
        auto res = stmt.exec(& storage_err);

        while (res.has_more()) {
            input_record in {res};
            std::string content_data;
            message::message_credentials m;

            success = in["message_id"]       >> m.id
                && in["author_id"]           >> m.author_id
                && in["creation_time"]       >> m.creation_time
                && in["local_creation_time"] >> m.creation_time
                && in["modification_time"]   >> m.modification_time
                && in["dispatched_time"]     >> m.dispatched_time
                && in["delivered_time"]      >> m.delivered_time
                && in["read_time"]           >> m.read_time
                && in["content"]             >> content_data;

            if (success) {
                error err;
                message::content content{content_data, & err};

                if (!content) {
                    if (perr) *perr = err;
                    return false;
                }

                m.contents = std::move(content);

                f(m);
            } else {
                break;
            }

            res.next();
        }

        if (res.is_error()) {
            success = false;
        }
    }

    if (storage_err) {
        error err {errc::storage_error
            , fmt::format("scan messages failure")
            , storage_err.what()};
        if (perr) *perr = err;
    }

    return success;
}


template <>
bool
conversation<BACKEND>::wipe (error * perr) noexcept
{
    debby::error storage_err;

    if (!_rep.dbh->remove(_rep.table_name, & storage_err)) {
        error err {errc::storage_error, fmt::format(WIPE_ERROR
            , to_string(_rep.addressee), storage_err.what())};
        if (perr) *perr = err;
        return false;
    }

    return true;
}

} // namespace chat
