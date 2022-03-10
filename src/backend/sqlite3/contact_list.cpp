////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.17 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/contact_list.hpp"
#include "pfs/chat/backend/sqlite3/contact_list.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <cassert>

namespace chat {

using namespace debby::sqlite3;

namespace {
    constexpr std::size_t CACHE_WINDOW_SIZE = 100;
} // namespace

static bool fill_contact (backend::sqlite3::db_traits::result_type * res
    , contact::contact * c)
{
    input_record in {*res};

    auto success = in["id"]  >> c->id
        && in["alias"]       >> c->alias
        && in["avatar"]      >> c->avatar
        && in["description"] >> c->description
        && in["type"]        >> c->type;

    return success;
}

namespace backend {
namespace sqlite3 {

void contact_list::invalidate_cache (rep_type * rep)
{
    rep->cache.dirty = true;
}

contact_list::rep_type
contact_list::make (shared_db_handle dbh
    , std::string const & table_name
    , error *)
{
    rep_type rep;
    rep.dbh = dbh;
    rep.table_name = table_name;

    invalidate_cache(& rep);

    return rep;
}

namespace {
std::string const SELECT_ROWS_RANGE {
    "SELECT `id`, `alias`, `avatar`, `description`, `type` FROM `{}` LIMIT {} OFFSET {}"
};
} // namespace

bool contact_list::prefetch (rep_type const * rep, int offset, int limit, error * perr)
{
    bool prefetch_required = rep->cache.dirty
        || offset < rep->cache.offset
        || offset + limit > rep->cache.offset + rep->cache.limit;

    if (!prefetch_required)
        return true;

    rep->cache.data.clear();
    rep->cache.map.clear();
    rep->cache.offset = offset;
    rep->cache.limit = 0;
    rep->cache.dirty = true;

    debby::error storage_err;
    auto stmt = rep->dbh->prepare(fmt::format(SELECT_ROWS_RANGE, rep->table_name
        , limit, offset), true, & storage_err);
    auto success = !!stmt;

    if (success) {
        auto res = stmt.exec(& storage_err);

        for (; success && res.has_more(); res.next()) {
            contact::contact c;
            success = fill_contact(& res, & c);

            if (success) {
                rep->cache.data.push_back(std::move(c));
                rep->cache.map.emplace(c.id, rep->cache.data.size() - 1);
                rep->cache.limit++;
            }
        }

        if (res.is_error())
            success = false;
    }

    if (success) {
        rep->cache.dirty = false;
    } else {
        auto err = error{errc::storage_error
            , "cache contacts failure"
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
    }

    return success;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::contact_list

template <>
contact_list<BACKEND>::contact_list (rep_type && rep)
    : _rep(std::move(rep))
{}

namespace {
std::string const COUNT_CONTACTS_BY_TYPE {
    "SELECT COUNT(1) as count FROM {} WHERE `type` = :type"
};
} // namespace

template <>
std::size_t
contact_list<BACKEND>::count () const
{
    return _rep.dbh->rows_count(_rep.table_name);
}

template <>
std::size_t
contact_list<BACKEND>::count (contact::type_enum type) const
{
    std::size_t count = 0;
    debby::error err;
    auto stmt = _rep.dbh->prepare(fmt::format(COUNT_CONTACTS_BY_TYPE, _rep.table_name)
        , true, & err);
    bool success = !!stmt;

    success = success && stmt.bind(":type" , to_storage(type), & err);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.has_more()) {
            auto opt = res.get<std::size_t>(0);
            assert(opt.has_value());
            count = *opt;
            res.next();
        }

        if (res.is_error())
            success = false;
    }

    return success ? count : 0;
}

namespace {
std::string const INSERT_CONTACT {
    "INSERT OR IGNORE INTO `{}` (`id`, `alias`, `avatar`, `description`, `type`)"
    " VALUES (:id, :alias, :avatar, :description, :type)"
};
} // namespace

template <>
int
contact_list<BACKEND>::add (contact::contact const & c, error * perr)
{
    BACKEND::invalidate_cache(& _rep);

    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_CONTACT, _rep.table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":id"    , to_storage(c.id), false, & storage_err)
        && stmt.bind(":alias" , to_storage(c.alias), false, & storage_err)
        && stmt.bind(":avatar", to_storage(c.avatar), false, & storage_err)
        && stmt.bind(":description", to_storage(c.description), false, & storage_err)
        && stmt.bind(":type"  , to_storage(c.type), & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , fmt::format("add contact failure: #{}", to_string(c.id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return -1;
    }

    return stmt.rows_affected();
}

// template <>
// int
// contact_list<BACKEND>::batch_add (std::function<bool()> has_next
//     , std::function<contact::contact()> next
//     , error * perr)
// {
//     int counter = 0;
//     bool success = _rep.dbh->begin();
//
//     if (success) {
//         error err;
//
//         while (!err && has_next()) {
//             auto n = add(next(), & err);
//             counter += n > 0 ? 1 : 0;
//         }
//
//         if (err) {
//             if (perr) *perr = err; else CHAT__THROW(err);
//             success = false;
//         }
//     }
//
//     if (success) {
//         _rep.dbh->commit();
//     } else {
//         _rep.dbh->rollback();
//         counter = -1;
//     }
//
//     return counter;
// }

namespace {

std::string const UPDATE_CONTACT {
    "UPDATE OR IGNORE `{}` SET `alias` = :alias, `avatar` = :avatar, `description` = :description"
    " WHERE `id` = :id AND `type` = :type"
};

} // namespace

template <>
int
contact_list<BACKEND>::update (contact::contact const & c, error * perr)
{
    BACKEND::invalidate_cache(& _rep);

    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(UPDATE_CONTACT, _rep.table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":alias" , to_storage(c.alias), false, & storage_err)
        && stmt.bind(":avatar", to_storage(c.avatar), false, & storage_err)
        && stmt.bind(":description", to_storage(c.description), false, & storage_err)
        && stmt.bind(":id"    , to_storage(c.id), false, & storage_err)
        && stmt.bind(":type"  , to_storage(c.type), & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , fmt::format("update contact failure: #{}", to_string(c.id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return -1;
    }

    return stmt.rows_affected();
}

namespace {
std::string const SELECT_CONTACT {
    "SELECT `id`, `alias`, `avatar`, `description`, `type` FROM `{}` WHERE `id` = :id"
};
} // namespace

template <>
contact::contact
contact_list<BACKEND>::get (contact::contact_id id, error * perr) const
{
    // Check cache
    if (!_rep.cache.dirty) {
        auto it = _rep.cache.map.find(id);
        if (it != _rep.cache.map.end()) {
            assert(it->second >= 0 && it->second < _rep.cache.data.size());
            return _rep.cache.data[it->second];
        }
    }

    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_CONTACT, _rep.table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success && stmt.bind(":id", to_storage(id), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.has_more()) {
            contact::contact c;
            success = fill_contact(& res, & c);

            if (success)
                return std::move(c);
        } else {
            auto err = error{errc::contact_not_found
                , fmt::format("contact not found by id: #{}", to_string(id))};
            if (perr) *perr = err; else CHAT__THROW(err);
            return contact::contact{};
        }

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , fmt::format("load contact failure: #{}", to_string(id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
    }

    return contact::contact{};
}

template <>
contact::contact
contact_list<BACKEND>::get (int offset, error * perr) const
{
    bool force_populate_cache = _rep.cache.dirty;

    // Out of range
    if (offset < _rep.cache.offset || offset >= _rep.cache.offset + _rep.cache.limit)
        force_populate_cache = true;

    // Populate cache if dirty
    if (force_populate_cache) {
        error err;
        auto success = BACKEND::prefetch(& _rep, offset, CACHE_WINDOW_SIZE, & err);

        if (!success) {
            if (perr) *perr = err; else CHAT__THROW(err);
            return contact::contact{};
        }
    }

    if (offset < _rep.cache.offset || offset >= _rep.cache.offset + _rep.cache.limit) {
        error err{errc::contact_not_found
            , fmt::format("contact not found by offset: {}", offset)};
        if (perr) *perr = err; else CHAT__THROW(err);
        return contact::contact{};
    }

    return _rep.cache.data[offset - _rep.cache.offset];
}

namespace {
std::string const SELECT_ALL_CONTACTS {
    "SELECT `id`, `alias`, `avatar`, `description`, `type` FROM `{}`"
};
} // namespace

template <>
bool
contact_list<BACKEND>::for_each (std::function<void(contact::contact const &)> f
    , error * perr)
{
    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _rep.table_name)
        , true, & storage_err);
    auto success = !!stmt;

    if (success) {
        auto res = stmt.exec(& storage_err);

        for (; success && res.has_more(); res.next()) {
            contact::contact c;
            success = fill_contact(& res, & c);

            if (success)
                f(c);
        }

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , "load contacts failure"
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
    }

    return success;
}

} // namespace chat
