////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.17 Refactored totally.
//      2023.04.23 Fixed according to new contact_list API.
//      2024.11.25 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "contact_list_impl.hpp"
#include "chat/contact_list.hpp"
#include <pfs/i18n.hpp>
#include <pfs/debby/data_definition.hpp>
#include <pfs/debby/relational_database.hpp>

namespace chat {

using relational_database_t = debby::relational_database<debby::backend_enum::sqlite3>;
using data_definition_t = debby::data_definition<debby::backend_enum::sqlite3>;

namespace storage {

sqlite3::contact_list * sqlite3::make_contact_list (std::string table_name
    , debby::relational_database<debby::backend_enum::sqlite3> & db)
{
    return new sqlite3::contact_list(table_name, db);
}

void sqlite3::contact_list::fill_contact (relational_database_t::result_type & result, contact::contact & c)
{
    c.contact_id  = result.get_or("id", contact::id{});
    c.creator_id  = result.get_or("creator_id", contact::id{});
    c.alias       = result.get_or("alias", std::string{});
    c.avatar      = result.get_or("avatar", std::string{});
    c.description = result.get_or("description", std::string{});
    c.extra       = result.get_or("extra", std::string{});
    c.type        = result.get_or("type", chat_enum::person);
}

static std::string const SELECT_ROWS_RANGE {
    "SELECT id, creator_id,  alias, avatar, description, extra, type"
    " FROM \"{}\" LIMIT {} OFFSET {}"
};

void sqlite3::contact_list::prefetch (int offset, int limit)
{
    bool prefetch_required = offset < cache.offset
        || offset + limit > cache.offset + cache.limit;

    if (!prefetch_required)
        return;

    cache.data.clear();
    cache.map.clear();
    cache.offset = offset;
    cache.limit = 0;

    debby::error err;
    auto stmt = pdb->prepare_cached(fmt::format(SELECT_ROWS_RANGE, table_name, limit, offset), & err);

    if (!err) {
        auto res = stmt.exec(& err);

        if (!err) {
            for (; res.has_more(); res.next()) {
                contact::contact c;
                fill_contact(res, c);

                cache.data.push_back(std::move(c));
                cache.map.emplace(c.contact_id, cache.data.size() - 1);
                cache.limit++;
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

} // namespace storage

using contact_list_t = contact_list<storage::sqlite3>;

template <>
contact_list_t::contact_list (rep * d) noexcept
    : _d(d)
{}

template <> contact_list_t::contact_list (contact_list && other) noexcept = default;
template <> contact_list_t & contact_list_t::operator = (contact_list && other) noexcept = default;
template <> contact_list_t::~contact_list () = default;

// This specialization not used
template <>
bool contact_list_t::add (contact::contact && /*c*/)
{
    return false;
}

template <>
std::size_t contact_list_t::count () const
{
    return _d->pdb->rows_count(_d->table_name);
}

template <>
std::size_t contact_list_t::count (chat_enum type) const
{
    static std::string const COUNT_CONTACTS_BY_TYPE {
        "SELECT COUNT(1) as count FROM \"{}\" WHERE type = {}"
    };

    debby::error err;
    auto res = _d->pdb->exec(fmt::format(COUNT_CONTACTS_BY_TYPE
        , _d->table_name, static_cast<std::underlying_type_t<chat_enum>>(type)), & err);

    if (err)
        throw error {errc::storage_error, err.what()};

    if (res.has_more()) {
        auto opt = res.get<std::size_t>(0, & err);

        if (err)
            throw error {errc::storage_error, err.what()};

        if (opt)
            return *opt;
    }

    throw error {errc::inconsistent_data, tr::_("unexpected result obtained while calculating number of contacts")};
    return 0;
}

template <>
contact::contact contact_list_t::get (contact::id id) const
{
    static char const * SELECT_CONTACT = "SELECT id, creator_id, alias"
        ", avatar, description, extra, type FROM \"{}\" WHERE id = :id";

    auto it = _d->cache.map.find(id);

    if (it != _d->cache.map.end()) {
        if (!(it->second >= 0 && it->second < _d->cache.data.size()))
            throw error {errc::inconsistent_data, tr::_("contact list cache corrupted")};

        return _d->cache.data[it->second];
    }

    debby::error err;

    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_CONTACT, _d->table_name), & err);

    if (!err) {
        stmt.bind(":id", id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more()) {
                    contact::contact c;
                    _d->fill_contact(res, c);
                    return c;
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    // Not found
    return contact::contact{};
}

template <>
contact::contact contact_list_t::at (int offset) const
{
    bool force_populate_cache = false;

    // Out of range
    if (offset < _d->cache.offset || offset >= _d->cache.offset + _d->cache.limit)
        force_populate_cache = true;

    // Populate cache if dirty
    if (force_populate_cache)
        _d->prefetch(offset, storage::sqlite3::cache_window_size());

    if (offset < _d->cache.offset || offset >= _d->cache.offset + _d->cache.limit)
        return contact::contact{};

    return _d->cache.data[offset - _d->cache.offset];
}

static char const * SELECT_ALL_CONTACTS = "SELECT id, creator_id"
    ", alias, avatar, description, extra, type FROM \"{}\"";

template <>
void contact_list_t::for_each (std::function<void(contact::contact const &)> f) const
{
    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ALL_CONTACTS, _d->table_name), & err);

    if (!err) {
        for (; res.has_more(); res.next()) {
            contact::contact c;
            _d->fill_contact(res, c);
            f(c);
        }
    } else {
        throw error {errc::storage_error, err.what()};
    }
}

template <>
void contact_list_t::for_each_until (std::function<bool(contact::contact const &)> f) const
{
    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ALL_CONTACTS, _d->table_name), & err);

    if (!err) {
        for (; res.has_more(); res.next()) {
            contact::contact c;
            _d->fill_contact(res, c);
            if (!f(c))
                break;
        }
    } else {
        throw error {errc::storage_error, err.what()};
    }
}

} // namespace chat
