////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.11 Initial version.
//      2024.12.01 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "chat/activity_manager.hpp"
#include "chat/error.hpp"
#include "chat/sqlite3.hpp"
#include <pfs/debby/data_definition.hpp>
#include <pfs/debby/sqlite3.hpp>

namespace chat {

using data_definition_t = debby::data_definition<debby::backend_enum::sqlite3>;
using activity_manager_t = activity_manager<storage::sqlite3>;

namespace storage {

std::function<std::string ()> sqlite3::activity_log_table_name = [] { return std::string{"activity_log"}; };
std::function<std::string ()> sqlite3::activity_brief_table_name = [] { return std::string{"activity_brief"}; };

class sqlite3::activity_manager
{
public:
    relational_database_t * pdb {nullptr};
    std::string log_table_name;
    std::string brief_table_name;

public:
    activity_manager (relational_database_t & db)
        : log_table_name(sqlite3::activity_log_table_name())
        , brief_table_name(sqlite3::activity_brief_table_name())
    {
        auto log = data_definition_t::create_table(log_table_name);
        log.add_column<contact::id>("contact_id");
        log.add_column<pfs::utc_time>("contact_activity");
        log.add_column<pfs::utc_time>("utc_time");

        auto brief = data_definition_t::create_table(brief_table_name);
        brief.add_column<contact::id>("contact_id").primary_key().unique();
        brief.add_column<pfs::utc_time>("online_utc_time").nullable();
        brief.add_column<pfs::utc_time>("offline_utc_time").nullable();
        brief.constraint("WITHOUT ROWID");

        auto brief_index = data_definition_t::create_index(brief_table_name + "_index");
        brief_index.on(brief_table_name).add_column("contact_id");

        std::array<std::string, 3> sqls = {
              log.build()
            , brief.build()
            , brief_index.build()
        };

        auto failure = db.transaction([& sqls, & db] () {
            debby::error err;

            for (auto const & sql: sqls) {
                db.query(sql, & err);

                if (err)
                    return pfs::make_optional(std::string{err.what()});
            }

            return pfs::optional<std::string>{};
        });

        if (failure) {
            throw error {
                  errc::storage_error
                , tr::_("create activity manager failure")
                , failure.value()
            };
        }

        pdb = & db;
    }
};

sqlite3::activity_manager *
sqlite3::make_activity_manager (debby::relational_database<debby::backend_enum::sqlite3> & db)
{
    return new sqlite3::activity_manager(db);
}

} // namespace storage

template <>
activity_manager_t::activity_manager (rep * d) noexcept
    : _d(d)
{}

template <> activity_manager_t::activity_manager (activity_manager && other) noexcept = default;
template <> activity_manager_t & activity_manager_t::operator = (activity_manager && other) noexcept = default;
template <> activity_manager_t::~activity_manager () = default;


template <>
activity_manager_t::operator bool () const noexcept
{
    return !!_d && _d->pdb != nullptr;
}

template <>
void activity_manager_t::clear ()
{
    std::array<std::string, 2> tables = {
          _d->log_table_name
        , _d->brief_table_name
    };

    auto failure = _d->pdb->transaction([this, & tables] {
        debby::error err;

        for (auto const & t: tables) {
            _d->pdb->clear(t, & err);

            if (err)
                return pfs::make_optional(std::string{err.what()});
        }

        return pfs::optional<std::string>{};
    });

    if (failure)
        throw error{errc::storage_error, failure.value()};
}

template <>
void activity_manager_t::log_activity (contact::id id
    , contact_activity ca, pfs::utc_time const & time, bool brief_only)
{
    static std::string const INSERT_LOG_RECORD {
        "INSERT INTO \"{}\" (contact_id, contact_activity, utc_time) VALUES (?, ?, ?)"
    };

    static std::string const UPDATE_BRIEF {
        "INSERT INTO \"{0}\" (contact_id, {1}) VALUES (:contact_id, :time)"
        " ON CONFLICT DO UPDATE SET {1}=:time WHERE contact_id=:contact_id"
    };

    debby::error err;

    _d->pdb->begin(& err);

    if (!err) {
        if (!brief_only) {
            auto stmt = _d->pdb->prepare_cached(fmt::format(INSERT_LOG_RECORD, _d->log_table_name), & err);

            stmt.bind(1, id, & err)
                && stmt.bind(2, ca, & err)
                && stmt.bind(3, time, & err);

            if (!err)
                stmt.exec(& err);
        }

        if (!err) {
            auto time_field_name = (ca == contact_activity::online)
                ? "online_utc_time"
                : "offline_utc_time";

            auto stmt = _d->pdb->prepare_cached(fmt::format(UPDATE_BRIEF, _d->brief_table_name
                , time_field_name), & err);

            if (!err) {
                stmt.bind(":contact_id", id, & err)
                    && stmt.bind(":time", time, & err);

                if (!err)
                    stmt.exec(& err);
            }
        }
    }

    if (!err) {
        _d->pdb->commit(& err);
    } else {
        _d->pdb->rollback();
        throw error{errc::storage_error, err.what()};
    }
}

template <>
pfs::optional<pfs::utc_time>
activity_manager_t::last_activity (contact::id id, contact_activity ca)
{
    static std::string const SELECT_ACTIVITY = { "SELECT {} FROM \"{}\" WHERE contact_id=?"};

    auto time_field_name = (ca == contact_activity::online)
        ? "online_utc_time"
        : "offline_utc_time";

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_ACTIVITY, time_field_name
        , _d->brief_table_name), & err);

    if (!err) {
        stmt.bind(1, id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more()) {
                    auto time = res.get<pfs::utc_time>(time_field_name);
                    return time;
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return pfs::nullopt;
}

template <>
activity_entry activity_manager_t::last_activity (contact::id id)
{
    static std::string const SELECT_ACTIVITY = {
        "SELECT * FROM \"{}\" WHERE contact_id=?"
    };

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_ACTIVITY, _d->brief_table_name), & err);

    if (!err) {
        stmt.bind(1, id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more()) {
                    auto offline_utc_time = res.get<pfs::utc_time>("offline_utc_time");
                    auto online_utc_time = res.get<pfs::utc_time>("online_utc_time");
                    return activity_entry{std::move(offline_utc_time), std::move(online_utc_time)};
                }
            }
        }
    }

    return activity_entry{pfs::nullopt, pfs::nullopt};
}

template <>
void activity_manager_t::clear_activities (contact::id id)
{
    static std::string const CLEAR_ACTIVITIES {
        "DELETE FROM \"{}\" WHERE contact_id = ?"
    };

    auto failure = _d->pdb->transaction([this, id] {
        debby::error err;

        auto stmt1 = _d->pdb->prepare(fmt::format(CLEAR_ACTIVITIES, _d->log_table_name), & err);

        if (err)
            return pfs::optional<std::string>{err.what()};

        auto stmt2 = _d->pdb->prepare(fmt::format(CLEAR_ACTIVITIES, _d->brief_table_name), & err);

        if (err)
            return pfs::optional<std::string>{err.what()};

        stmt1.bind(1, id, & err)
            && stmt2.bind(1, id, & err);

        if (err)
            return pfs::optional<std::string>{err.what()};

        for (auto * stmt: {& stmt1, & stmt2}) {
            auto res = stmt->exec(& err);

            if (err)
                return pfs::optional<std::string>{err.what()};
        }

        return pfs::optional<std::string>{};
    });

    if (failure)
        throw error{errc::storage_error, failure.value()};
}

template <>
void activity_manager_t::clear_activities ()
{
    static std::string const CLEAR_ALL_ACTIVITIES { "DELETE FROM \"{}\"" };

    auto failure = _d->pdb->transaction([this] {
        debby::error err;

        auto stmt1 = _d->pdb->exec(fmt::format(CLEAR_ALL_ACTIVITIES, _d->log_table_name), & err);

        if (err)
            return pfs::optional<std::string>{err.what()};

        auto stmt2 = _d->pdb->exec(fmt::format(CLEAR_ALL_ACTIVITIES, _d->brief_table_name), & err);

        if (err)
            return pfs::optional<std::string>{err.what()};

        return pfs::optional<std::string>{};
    });

    if (failure)
        throw error{errc::storage_error, failure.value()};
}

template <>
void activity_manager_t::for_each_activity (contact::id id
    , std::function<void(contact_activity ca, pfs::utc_time const &)> f)
{
    static std::string const SELECT_ACTIVITY = {
        "SELECT contact_activity, utc_time FROM \"{}\" WHERE contact_id=? ORDER BY utc_time ASC"
    };

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_ACTIVITY, _d->log_table_name), & err);

    if (!err) {
        stmt.bind(1, id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                while (res.has_more()) {
                    auto ca = res.get<contact_activity>("contact_activity");
                    auto time = res.get<pfs::utc_time>("utc_time");

                    if (ca && time)
                        f(*ca, *time);

                    res.next();
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

template <>
void activity_manager_t::for_each_activity (std::function<void(contact::id, contact_activity ca
    , pfs::utc_time const &)> f)
{
    static std::string const SELECT_ACTIVITY = {
        "SELECT contact_id, contact_activity, utc_time FROM \"{}\" ORDER BY utc_time ASC"
    };

    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ACTIVITY, _d->log_table_name), & err);

    if (!err) {
        while (res.has_more()) {
            auto id = res.get<contact::id>("contact_id");
            auto ca = res.get<contact_activity>("contact_activity");
            auto time = res.get<pfs::utc_time>("utc_time");

            if (id && ca && time)
                f(*id, *ca, *time);

            res.next();
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

template <>
void activity_manager_t::for_each_activity_brief (
      std::function<void(contact::id, pfs::optional<pfs::utc_time> const & online_utc_time
        , pfs::optional<pfs::utc_time> const & offline_utc_time)> f)
{
    static std::string const SELECT_ACTIVITY_BRIEF = {
        "SELECT contact_id, online_utc_time, offline_utc_time FROM \"{}\""
    };

    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ACTIVITY_BRIEF, _d->brief_table_name), & err);

    if (!err) {
        while (res.has_more()) {
            auto id = res.get<contact::id>("contact_id");
            auto online_utc_time = res.get<pfs::utc_time>("online_utc_time");
            auto offline_utc_time = res.get<pfs::utc_time>("offline_utc_time");

            if (id)
                f(*id, online_utc_time, offline_utc_time);

            res.next();
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

} // namespace chat
