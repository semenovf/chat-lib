////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/persistent_storage/sqlite3/contact_list.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <cassert>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;

namespace {
    constexpr std::size_t CACHE_WINDOW_SIZE = 100;
} // namespace

contact_list::contact_list (database_handle_t dbh
    , std::string const & table_name)
    : _dbh(dbh)
    , _table_name(table_name)
{
    invalidate_cache();
}

namespace {
std::string const COUNT_CONTACTS_BY_TYPE {
    "SELECT COUNT(1) as count FROM {} WHERE `type` = :type"
};
} // namespace

std::size_t contact_list::count_impl () const
{
    return _dbh->rows_count(_table_name);
}

std::size_t contact_list::count_impl (contact::type_enum type) const
{
    std::size_t count = 0;
    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(COUNT_CONTACTS_BY_TYPE, _table_name)
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
    "INSERT OR IGNORE INTO `{}` (`id`, `alias`, `avatar`, `type`)"
    " VALUES (:id, :alias, :avatar, :type)"
};
} // namespace

int contact_list::add_impl (contact::contact const & c, error * perr)
{
    invalidate_cache();

    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(INSERT_CONTACT, _table_name), true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":id"    , to_storage(c.id), false, & storage_err)
        && stmt.bind(":alias" , to_storage(c.alias), false, & storage_err)
        && stmt.bind(":avatar", to_storage(c.avatar), false, & storage_err)
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

namespace {

std::string const UPDATE_CONTACT {
    "UPDATE OR IGNORE `{}` SET `alias` = :alias, `avatar` = :avatar"
    " WHERE `id` = :id AND `type` = :type"
};

} // namespace

int contact_list::update_impl (contact::contact const & c, error * perr)
{
    invalidate_cache();

    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(UPDATE_CONTACT, _table_name), true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":alias" , to_storage(c.alias), false, & storage_err)
        && stmt.bind(":avatar", to_storage(c.avatar), false, & storage_err)
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
    "SELECT `id`, `alias`, `avatar`, `type` FROM `{}` WHERE `id` = :id"
};
} // namespace

bool contact_list::fill_contact (result_t * res, contact::contact * c) const
{
    input_record in {*res};

    auto success = in["id"] >> c->id
        && in["alias"]      >> c->alias
        && in["avatar"]     >> c->avatar
        && in["type"]       >> c->type;

    return success;
}

pfs::optional<contact::contact> contact_list::get_impl (contact::contact_id id
    , error * perr) const
{
    // Check cache
    if (!_cache.dirty) {
        auto it = _cache.map.find(id);
        if (it != _cache.map.end()) {
            assert(it->second >= 0 && it->second < _cache.data.size());
            return _cache.data[it->second];
        }
    }

    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_CONTACT, _table_name), true, & storage_err);
    bool success = !!stmt;

    success = success && stmt.bind(":id", to_storage(id), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.has_more()) {
            contact::contact c;
            success = fill_contact(& res, & c);

            if (success)
                return std::move(c);
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

    return pfs::nullopt;
}

pfs::optional<contact::contact> contact_list::get_impl (int offset, error * perr) const
{
    bool force_populate_cache = _cache.dirty;

    // Out of range
    if (offset < _cache.offset || offset >= _cache.offset + _cache.limit)
        force_populate_cache = true;

    // Populate cache if dirty
    if (force_populate_cache) {
        auto success = prefetch(offset, CACHE_WINDOW_SIZE, perr);

        if (!success)
            return pfs::nullopt;
    }

    if (offset < _cache.offset || offset >= _cache.offset + _cache.limit)
        return pfs::nullopt;

    return _cache.data[offset - _cache.offset];
}

namespace {

std::string const SELECT_ALL_CONTACTS {
    "SELECT `id`, `alias`, `avatar`, `type` FROM `{}`"
};

} // namespace

bool contact_list::all_of_impl (std::function<void(contact::contact const &)> f
    , error * perr)
{
    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _table_name)
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

namespace {

std::string const SELECT_ROWS_RANGE {
    "SELECT `id`, `alias`, `avatar`, `type` FROM `{}` LIMIT {} OFFSET {}"
};

} // namespace

void contact_list::invalidate_cache ()
{
    _cache.dirty = true;
}

bool contact_list::prefetch (int offset, int limit, error * perr) const
{
    bool prefetch_required = _cache.dirty
        || offset < _cache.offset
        || offset + limit > _cache.offset + _cache.limit;

    if (!prefetch_required)
        return true;

    _cache.data.clear();
    _cache.map.clear();
    _cache.offset = offset;
    _cache.limit = 0;
    _cache.dirty = true;

    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_ROWS_RANGE, _table_name
        , limit, offset), true, & storage_err);
    auto success = !!stmt;

    if (success) {
        auto res = stmt.exec(& storage_err);

        for (; success && res.has_more(); res.next()) {
            contact::contact c;
            success = fill_contact(& res, & c);

            if (success) {
                _cache.data.push_back(std::move(c));
                _cache.map.emplace(c.id, _cache.data.size() - 1);
                _cache.limit++;
            }
        }

        if (res.is_error())
            success = false;
    }

    if (success) {
        _cache.dirty = false;
    } else {
        auto err = error{errc::storage_error
            , "cache contacts failure"
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
    }

    return success;
}

}}} // namespace chat::persistent_storage::sqlite3
