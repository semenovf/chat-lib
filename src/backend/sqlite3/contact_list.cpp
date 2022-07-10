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
#include "pfs/chat/error.hpp"
#include "pfs/chat/backend/sqlite3/contact_list.hpp"
#include "pfs/debby/backend/sqlite3/time_point_traits.hpp"
#include "pfs/debby/backend/sqlite3/uuid_traits.hpp"
#include <cassert>

namespace chat {

using namespace debby::backend::sqlite3;

constexpr std::size_t CACHE_WINDOW_SIZE = 100;

static void fill_contact (backend::sqlite3::db_traits::result_type & result
    , contact::contact & c)
{
    result["id"]          >> c.id;
    result["creator_id"]  >> c.creator_id;
    result["alias"]       >> c.alias;
    result["avatar"]      >> c.avatar;
    result["description"] >> c.description;
    result["type"]        >> c.type;
}

namespace backend {
namespace sqlite3 {

std::atomic<bool> contact_list::in_memory_cache::dirty {true};

void contact_list::invalidate_cache (rep_type * rep)
{
    rep->cache.dirty = true;
}

contact_list::rep_type
contact_list::make (shared_db_handle dbh
    , std::string const & table_name)
{
    rep_type rep;
    rep.dbh = dbh;
    rep.table_name = table_name;

    invalidate_cache(& rep);

    return rep;
}

static std::string const SELECT_ROWS_RANGE {
    "SELECT `id`, `creator_id`,  `alias`, `avatar`, `description`, `type`"
    " FROM `{}`"
    " {}"        // ORDER BY ...
    " LIMIT {} OFFSET {}"
};

void contact_list::prefetch (rep_type const * rep, int offset, int limit, int sort_flags)
{
    bool prefetch_required = rep->cache.dirty
        || offset < rep->cache.offset
        || offset + limit > rep->cache.offset + rep->cache.limit
        || sort_flags != rep->cache.sort_flags;

    if (!prefetch_required)
        return;

    rep->cache.data.clear();
    rep->cache.map.clear();
    rep->cache.offset = offset;
    rep->cache.limit = 0;
    rep->cache.dirty = true;
    rep->cache.sort_flags = sort_flags;

    std::string order_by {"ORDER BY "};

    if (sort_flag_on(sort_flags, contact_sort_flag::by_alias))
        order_by += "`alias`";
    else
        order_by.clear();

    if (!order_by.empty()) {
        if (sort_flag_on(sort_flags, contact_sort_flag::ascending_order))
            order_by += " ASC";
        else if (sort_flag_on(sort_flags, contact_sort_flag::descending_order))
            order_by += " DESC";
        else 
            order_by += " ASC";
    }

    auto stmt = rep->dbh->prepare(
          fmt::format(SELECT_ROWS_RANGE, rep->table_name
        , order_by, limit, offset));

    CHAT__ASSERT(!!stmt, "");

    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        contact::contact c;
        fill_contact(res, c);

        rep->cache.data.push_back(std::move(c));
        rep->cache.map.emplace(c.id, rep->cache.data.size() - 1);
        rep->cache.limit++;
    }

    rep->cache.dirty = false;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::contact_list;

template <>
contact_list<BACKEND>::contact_list (rep_type && rep)
    : _rep(std::move(rep))
{}

static std::string const COUNT_CONTACTS_BY_TYPE {
    "SELECT COUNT(1) as count FROM {} WHERE `type` = :type"
};

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
    auto stmt = _rep.dbh->prepare(fmt::format(COUNT_CONTACTS_BY_TYPE, _rep.table_name));
    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":type", to_storage(type));

    auto res = stmt.exec();

    if (res.has_more()) {
        count = res.get<std::size_t>(0);
        res.next();
    }

    return count;
}

namespace {
std::string const INSERT_CONTACT {
    "INSERT OR IGNORE INTO `{}` (`id`, `creator_id`, `alias`, `avatar`"
    ", `description`, `type`)"
    " VALUES (:id, :creator_id, :alias, :avatar, :description, :type)"
};
} // namespace

template <>
int
contact_list<BACKEND>::add (contact::contact const & c)
{
    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_CONTACT, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":id"         , to_storage(c.id));
    stmt.bind(":creator_id" , to_storage(c.creator_id));
    stmt.bind(":alias"      , to_storage(c.alias));
    stmt.bind(":avatar"     , to_storage(c.avatar));
    stmt.bind(":description", to_storage(c.description));
    stmt.bind(":type"       , to_storage(c.type));

    auto res = stmt.exec();
    auto n = stmt.rows_affected();

    if (n > 0)
        BACKEND::invalidate_cache(& _rep);

    return n;
}

namespace {

std::string const UPDATE_CONTACT {
    "UPDATE OR IGNORE `{}` SET `alias` = :alias, `avatar` = :avatar, `description` = :description"
    " WHERE `id` = :id AND `type` = :type"
};

} // namespace

template <>
int
contact_list<BACKEND>::update (contact::contact const & c)
{
    auto stmt = _rep.dbh->prepare(fmt::format(UPDATE_CONTACT, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":alias" , to_storage(c.alias));
    stmt.bind(":avatar", to_storage(c.avatar));
    stmt.bind(":description", to_storage(c.description));
    stmt.bind(":id", to_storage(c.id));
    stmt.bind(":type", to_storage(c.type));

    auto res = stmt.exec();
    auto n = stmt.rows_affected();

    if (n > 0)
        BACKEND::invalidate_cache(& _rep);

    return n;
}

namespace {
std::string const REMOVE_CONTACT {
    "DELETE from `{}` WHERE `id` = :id"
};
} // namespace

template <>
void
contact_list<BACKEND>::remove (contact::contact_id id)
{
    auto stmt = _rep.dbh->prepare(fmt::format(REMOVE_CONTACT, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":id", id);

    auto res = stmt.exec();
    BACKEND::invalidate_cache(& _rep);
}

namespace {
std::string const SELECT_CONTACT {
    "SELECT `id`, `creator_id`, `alias`, `avatar`, `description`, `type`"
    " FROM `{}` WHERE `id` = :id"
};
} // namespace

template <>
contact::contact
contact_list<BACKEND>::get (contact::contact_id id) const
{
    // Check cache
    if (!_rep.cache.dirty) {
        auto it = _rep.cache.map.find(id);
        if (it != _rep.cache.map.end()) {
            assert(it->second >= 0 && it->second < _rep.cache.data.size());
            return _rep.cache.data[it->second];
        }
    }

    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_CONTACT, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":id", id);

    auto res = stmt.exec();

    if (res.has_more()) {
        contact::contact c;
        fill_contact(res, c);
        return std::move(c);
    }

    return contact::contact{};
}

template <>
contact::contact
contact_list<BACKEND>::get (int offset, int sort_flag) const
{
    bool force_populate_cache = _rep.cache.dirty;

    // Out of range
    if (offset < _rep.cache.offset || offset >= _rep.cache.offset + _rep.cache.limit)
        force_populate_cache = true;

    // Populate cache if dirty
    if (force_populate_cache) {
        BACKEND::prefetch(& _rep, offset, CACHE_WINDOW_SIZE, sort_flag);
    }

    if (offset < _rep.cache.offset || offset >= _rep.cache.offset + _rep.cache.limit) {
        return contact::contact{};
    }

    return _rep.cache.data[offset - _rep.cache.offset];
}

namespace {
std::string const SELECT_ALL_CONTACTS {
    "SELECT `id`, `creator_id`, `alias`, `avatar`, `description`, `type` FROM `{}`"
};
} // namespace

template <>
void
contact_list<BACKEND>::for_each (std::function<void(contact::contact const &)> f)
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        contact::contact c;
        fill_contact(res, c);
        f(c);
    }
}

template <>
void
contact_list<BACKEND>::for_each_until (std::function<bool(contact::contact const &)> f)
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        contact::contact c;
        fill_contact(res, c);
        if (!f(c))
            break;
    }
}

} // namespace chat
