////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/activity_manager.hpp"
#include "pfs/chat/error.hpp"
#include "pfs/chat/backend/sqlite3/activity_manager.hpp"
#include "pfs/debby/backend/sqlite3/time_point_traits.hpp"
#include "pfs/debby/backend/sqlite3/uuid_traits.hpp"

namespace chat {

using namespace debby::backend::sqlite3;

static std::string const DEFAULT_ACTIVITY_LOG_TABLE_NAME   { "activity_log" };
static std::string const DEFAULT_ACTIVITY_BRIEF_TABLE_NAME { "activity_brief" };

namespace backend {
namespace sqlite3 {

activity_manager::rep_type
activity_manager::make (shared_db_handle dbh)
{
    static std::string const CREATE_ACTIVITY_LOG_TABLE {
        "CREATE TABLE IF NOT EXISTS `{}` ("
        "contact_id {} NOT NULL"
        ", contact_activity {} NOT NULL"
        ", utc_time {} NO NULL)"

    };

    static std::string const CREATE_ACTIVITY_BRIEF_TABLE {
        "CREATE TABLE IF NOT EXISTS `{}` ("
        "contact_id {} UNIQUE NOT NULL"
        ", online_utc_time {}"
        ", offline_utc_time {}"
        ", PRIMARY KEY(contact_id)) WITHOUT ROWID"
    };

    static std::string const CREATE_ACTIVITY_BRIEF_INDEX {
        "CREATE INDEX IF NOT EXISTS `{0}_index` ON `{0}` (`contact_id`)"
    };

    rep_type rep;
    rep.dbh = dbh;
    rep.log_table_name = DEFAULT_ACTIVITY_LOG_TABLE_NAME;
    rep.brief_table_name = DEFAULT_ACTIVITY_BRIEF_TABLE_NAME;

    std::array<std::string, 3> sqls = {
          fmt::format(CREATE_ACTIVITY_LOG_TABLE
            , rep.log_table_name
            , affinity_traits<contact::id>::name()
            , affinity_traits<contact_activity>::name()
            , affinity_traits<pfs::utc_time>::name())
        , fmt::format(CREATE_ACTIVITY_BRIEF_TABLE
            , rep.brief_table_name
            , affinity_traits<contact::id>::name()
            , affinity_traits<pfs::utc_time>::name()
            , affinity_traits<pfs::utc_time>::name())
        , fmt::format(CREATE_ACTIVITY_BRIEF_INDEX, rep.brief_table_name)
    };

    try {
        rep.dbh->begin();

        for (auto const & sql: sqls)
            rep.dbh->query(sql);

        rep.dbh->commit();
    } catch (debby::error ex) {
        rep.dbh->rollback();

        shared_db_handle empty;
        rep.dbh.swap(empty);
        throw error{errc::storage_error, ex.what()};
    }

    return rep;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::activity_manager;

template <>
activity_manager<BACKEND>::activity_manager (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
activity_manager<BACKEND>::operator bool () const noexcept
{
    return !!_rep.dbh;
}

template <>
void
activity_manager<BACKEND>::wipe (chat::error * perr)
{
    std::array<std::string, 2> tables = {
          _rep.log_table_name
        , _rep.brief_table_name
    };

    try {
        _rep.dbh->begin();

        for (auto const & t: tables) {
            _rep.dbh->clear(t);
        }

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();

        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

template <>
void
activity_manager<BACKEND>::log_activity (contact::id id
    , contact_activity ca, pfs::utc_time const & time, bool brief_only, error * perr)
{
    static std::string const INSERT_LOG_RECORD {
        "INSERT INTO `{}` (`contact_id`, `contact_activity`, `utc_time`) VALUES (?, ?, ?)"
    };

    static std::string const UPDATE_BRIEF {
        "INSERT INTO `{0}` (contact_id, {1}) VALUES (:contact_id, :time)"
        " ON CONFLICT DO UPDATE SET {1}=:time WHERE contact_id=:contact_id"
    };

    try {
        _rep.dbh->begin();

        if (!brief_only) {
            auto stmt = _rep.dbh->prepare(fmt::format(INSERT_LOG_RECORD
                , _rep.log_table_name));

            stmt.bind(0, id);
            stmt.bind(1, ca);
            stmt.bind(2, time);
            stmt.exec();
        }

        auto time_field_name = (ca == contact_activity::online)
            ? "online_utc_time"
            : "offline_utc_time";

        auto stmt = _rep.dbh->prepare(fmt::format(UPDATE_BRIEF
            , _rep.brief_table_name, time_field_name));

        stmt.bind(":contact_id", id);
        stmt.bind(":time", time);
        stmt.exec();

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();

        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

template <>
pfs::optional<pfs::utc_time>
activity_manager<BACKEND>::last_activity (contact::id id, contact_activity ca
    , error * perr)
{
    static std::string const SELECT_ACTIVITY = {
        "SELECT {} FROM `{}` WHERE contact_id=?"
    };

    auto time_field_name = (ca == contact_activity::online)
        ? "online_utc_time"
        : "offline_utc_time";

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ACTIVITY
            , time_field_name, _rep.brief_table_name));
        stmt.bind(0, id);

        auto res = stmt.exec();

        if (res.has_more()) {
            pfs::optional<pfs::utc_time> time;
            res[time_field_name] >> time;
            return time;
        }
    } catch (debby::error ex) {
        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }

    return pfs::nullopt;
}

template <>
activity_entry
activity_manager<BACKEND>::last_activity (contact::id id, error * perr)
{
    static std::string const SELECT_ACTIVITY = {
        "SELECT * FROM `{}` WHERE contact_id=?"
    };

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ACTIVITY
            , _rep.brief_table_name));
        stmt.bind(0, id);

        auto res = stmt.exec();

        if (res.has_more()) {
            pfs::optional<pfs::utc_time> offline_utc_time;
            pfs::optional<pfs::utc_time> online_utc_time;
            res["offline_utc_time"] >> offline_utc_time;
            res["online_utc_time"] >> online_utc_time;
            return activity_entry{std::move(offline_utc_time), std::move(online_utc_time)};
        }
    } catch (debby::error ex) {
        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }

    return activity_entry{pfs::nullopt, pfs::nullopt};
}

template <>
void
activity_manager<BACKEND>::clear_activities (contact::id id, error * perr)
{
    static std::string const CLEAR_ACTIVITIES {
        "DELETE from `{}` WHERE contact_id = ?"
    };

    auto stmt1 = _rep.dbh->prepare(fmt::format(CLEAR_ACTIVITIES, _rep.log_table_name));
    auto stmt2 = _rep.dbh->prepare(fmt::format(CLEAR_ACTIVITIES, _rep.brief_table_name));

    stmt1.bind(0, id);
    stmt2.bind(0, id);

    try {
        _rep.dbh->begin();

        for (auto * stmt: {& stmt1, & stmt2}) {
            auto res = stmt->exec();
        }

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();

        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

template <>
void
activity_manager<BACKEND>::clear_activities (error * perr)
{
    static std::string const CLEAR_ALL_ACTIVITIES { "DELETE from `{}`" };

    auto stmt1 = _rep.dbh->prepare(fmt::format(CLEAR_ALL_ACTIVITIES, _rep.log_table_name));
    auto stmt2 = _rep.dbh->prepare(fmt::format(CLEAR_ALL_ACTIVITIES, _rep.brief_table_name));

    try {
        _rep.dbh->begin();

        for (auto * stmt: {& stmt1, & stmt2}) {
            auto res = stmt->exec();
        }

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();

        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

template <>
void
activity_manager<BACKEND>::for_each_activity (contact::id id
    , std::function<void(contact_activity ca, pfs::utc_time const &)> f
    , error * perr)
{
    static std::string const SELECT_ACTIVITY = {
        "SELECT contact_activity, utc_time FROM `{}` WHERE contact_id=? ORDER BY utc_time ASC"
    };

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ACTIVITY
            , _rep.log_table_name));
        stmt.bind(0, id);

        auto res = stmt.exec();

        while (res.has_more()) {
            contact_activity ca;
            pfs::utc_time time;

            res["contact_activity"] >> ca;
            res["utc_time"] >> time;

            f(ca, time);

            res.next();
        }
    } catch (debby::error ex) {
        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

template <>
void
activity_manager<BACKEND>::for_each_activity (
      std::function<void(contact::id, contact_activity ca, pfs::utc_time const &)> f
    , error * perr)
{
    static std::string const SELECT_ACTIVITY = {
        "SELECT contact_id, contact_activity, utc_time FROM `{}` ORDER BY utc_time ASC"
    };

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ACTIVITY
            , _rep.log_table_name));

        auto res = stmt.exec();

        while (res.has_more()) {
            contact::id id;
            contact_activity ca;
            pfs::utc_time time;

            res["contact_id"] >> id;
            res["contact_activity"] >> ca;
            res["utc_time"] >> time;

            f(id, ca, time);

            res.next();
        }
    } catch (debby::error ex) {
        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

template <>
void
activity_manager<BACKEND>::for_each_activity_brief (
    std::function<void(contact::id, pfs::optional<pfs::utc_time> const & online_utc_time
        , pfs::optional<pfs::utc_time> const & offline_utc_time)> f, error * perr)
{
    static std::string const SELECT_ACTIVITY_BRIEF = {
        "SELECT contact_id, online_utc_time, offline_utc_time FROM `{}`"
    };

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ACTIVITY_BRIEF
            , _rep.brief_table_name));

        auto res = stmt.exec();

        while (res.has_more()) {
            contact::id id;
            pfs::optional<pfs::utc_time> online_utc_time;
            pfs::optional<pfs::utc_time> offline_utc_time;

            res["contact_id"] >> id;
            res["online_utc_time"] >> online_utc_time;
            res["offline_utc_time"] >> offline_utc_time;

            f(id, online_utc_time, offline_utc_time);

            res.next();
        }
    } catch (debby::error ex) {
        error err {errc::storage_error, ex.what()};

        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

} // namespace chat
