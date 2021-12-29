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

    std::string const ADD_CONTACT_ERROR       { "add contact failure: {}" };
    std::string const UPDATE_CONTACT_ERROR    { "update contact failure: {}" };
    std::string const LOAD_CONTACT_ERROR      { "load contact failure: {}" };
    std::string const LOAD_ALL_CONTACTS_ERROR { "load contacts failure: {}" };
    std::string const CACHE_CONTACTS_ERROR    { "cache contacts failure: {}"};
} // namespace

contact_list::contact_list (database_handle_t dbh
    , std::string const & table_name
    , failure_handler_t & f)
    : _dbh(dbh)
    , _table_name(table_name)
    , _on_failure(f)
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

    return count;
}

namespace {
std::string const INSERT_CONTACT {
    "INSERT OR IGNORE INTO `{}` (`id`, `alias`, `type`)"
    " VALUES (:id, :alias, :type)"
};
} // namespace

int contact_list::add_impl (contact::contact const & c)
{
    invalidate_cache();

    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(INSERT_CONTACT, _table_name), true, & err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":id"   , to_storage(c.id), false, & err)
        && stmt.bind(":alias", to_storage(c.alias), false, & err)
        && stmt.bind(":type" , to_storage(c.type), & err);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.is_error())
            success = false;
    }

    if (!success)
        _on_failure(fmt::format(ADD_CONTACT_ERROR, err.what()));

    return success ? stmt.rows_affected() : -1;
}

namespace {

std::string const UPDATE_CONTACT {
    "UPDATE OR IGNORE `{}` SET `alias` = :alias"
    " WHERE `id` = :id AND `type` = :type"
};

} // namespace

int contact_list::update_impl (contact::contact const & c)
{
    invalidate_cache();

    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(UPDATE_CONTACT, _table_name), true, & err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":alias", to_storage(c.alias), false, & err)
        && stmt.bind(":id", to_storage(c.id), false, & err)
        && stmt.bind(":type", to_storage(c.type), & err);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.is_error())
            success = false;
    }

    if (!success)
        _on_failure(fmt::format(UPDATE_CONTACT_ERROR, err.what()));

    return success ? stmt.rows_affected() : -1;
}

namespace {
std::string const SELECT_CONTACT {
    "SELECT `id`, `alias`, `type` FROM `{}` WHERE `id` = :id"
};
} // namespace

bool contact_list::fill_contact (result_t * res, contact::contact * c)
{
    input_record in {*res};

    auto success = in["id"] >> c->id
        && in["alias"]      >> c->alias
        && in["type"]       >> c->type;

    return success;
}

pfs::optional<contact::contact> contact_list::get_impl (contact::contact_id id)
{
    // Check cache
    if (!_cache.dirty) {
        auto it = _cache.map.find(id);
        if (it != _cache.map.end()) {
            assert(it->second >= 0 && it->second < _cache.data.size());
            return _cache.data[it->second];
        }
    }

    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_CONTACT, _table_name), true, & err);
    bool success = !!stmt;

    success = success && stmt.bind(":id", to_storage(id), false, & err);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.has_more()) {
            contact::contact c;
            success = fill_contact(& res, & c);

            if (success)
                return std::move(c);
        }

        if (res.is_error())
            success = false;
    }

    if (!success)
        _on_failure(fmt::format(LOAD_CONTACT_ERROR, err.what()));

    return pfs::nullopt;
}

pfs::optional<contact::contact> contact_list::get_impl (int offset)
{
    bool force_populate_cache = _cache.dirty;

    // Out of range
    if (offset < _cache.offset || offset >= _cache.offset + _cache.limit)
        force_populate_cache = true;

    // Populate cache if dirty
    if (force_populate_cache) {
        auto success = prefetch(offset, CACHE_WINDOW_SIZE);

        if (!success)
            return pfs::nullopt;
    }

    if (offset < _cache.offset || offset >= _cache.offset + _cache.limit)
        return pfs::nullopt;

    return _cache.data[offset - _cache.offset];
}

namespace {

std::string const SELECT_ALL_CONTACTS {
    "SELECT `id`, `alias`, `type` FROM `{}`"
};

} // namespace

bool contact_list::all_of_impl (std::function<void(contact::contact const &)> f)
{
    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _table_name), true, & err);
    auto success = !!stmt;

    if (success) {
        auto res = stmt.exec(& err);

        for (; success && res.has_more(); res.next()) {
            contact::contact c;
            success = fill_contact(& res, & c);

            if (success)
                f(c);
        }

        if (res.is_error())
            success = false;
    }

    if (!success)
        _on_failure(fmt::format(LOAD_ALL_CONTACTS_ERROR, err.what()));

    return success;
}

namespace {

std::string const SELECT_ROWS_RANGE {
    "SELECT `id`, `alias`, `type` FROM `{}` LIMIT {} OFFSET {}"
};

} // namespace

void contact_list::invalidate_cache ()
{
    _cache.dirty = true;
}

int contact_list::prefetch (int offset, int limit)
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

    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_ROWS_RANGE, _table_name
        , limit, offset), true, & err);
    auto success = !!stmt;

    if (success) {
        auto res = stmt.exec(& err);

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
        _on_failure(fmt::format(CACHE_CONTACTS_ERROR, err.what()));
    }

    return success;
}

}}} // namespace chat::persistent_storage::sqlite3
