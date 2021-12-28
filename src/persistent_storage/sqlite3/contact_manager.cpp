////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/persistent_storage/sqlite3/contact_manager.hpp"
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

    std::string const DEFAULT_CONTACTS_TABLE_NAME  { "chat_contacts" };
    std::string const DEFAULT_MEMBERS_TABLE_NAME   { "chat_groups" };
    std::string const DEFAULT_FOLLOWERS_TABLE_NAME { "chat_channels" };

    std::string const INIT_CONTACT_MANAGER_ERROR { "initialization contact manager failure: {}" };
//     std::string const COUNT_CONTACTS_ERROR    { "count contacts failure: {}" };
//     std::string const ADD_CONTACT_ERROR       { "add contact failure: {}" };
//     std::string const UPDATE_CONTACT_ERROR    { "update contact failure: {}" };
//     std::string const LOAD_CONTACT_ERROR      { "load contact failure: {}" };
//     std::string const LOAD_ALL_CONTACTS_ERROR { "load contacts failure: {}" };
//     std::string const CACHE_CONTACTS_ERROR    { "cache contacts failure: {}"};
    std::string const WIPE_ERROR              { "wipe contact data failure: {}" };
} // namespace

namespace {

std::string const CREATE_CONTACTS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`id` {} NOT NULL UNIQUE"
        ", `alias` {} NOT NULL"
        ", `type` {} NOT NULL"
        ", PRIMARY KEY(`id`)) WITHOUT ROWID"
};

std::string const CREATE_MEMBERS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`group_id` {} NOT NULL"
        ", `contact_id` {} NOT NULL)"
};

std::string const CREATE_FOLLOWERS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`channel_id` {} NOT NULL"
        ", `contact_id` {} NOT NULL)"
};

std::string const CREATE_CONTACTS_INDEX {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{0}_index` ON `{0}` (`id`)"
};

std::string const CREATE_MEMBERS_INDEX {
    "CREATE INDEX IF NOT EXISTS `{0}_index` ON `{0}` (`group_id`)"
};

std::string const CREATE_FOLLOWERS_INDEX {
    "CREATE INDEX IF NOT EXISTS `{0}_index` ON `{0}` (`channel_id`)"
};

} // namespace

contact_manager::contact_manager (database_handle_t dbh
    , std::function<void(std::string const &)> f)
    : base_class(f)
    , _dbh(dbh)
    , _contacts_table_name(DEFAULT_CONTACTS_TABLE_NAME)
    , _members_table_name(DEFAULT_MEMBERS_TABLE_NAME)
    , _followers_table_name(DEFAULT_FOLLOWERS_TABLE_NAME)
{
    // FIXME
//     invalidate_cache();

    std::array<std::string, 6> sqls = {
          fmt::format(CREATE_CONTACTS_TABLE
            , _contacts_table_name
            , affinity_traits<contact::contact_id>::name()
            , affinity_traits<decltype(contact::contact_credentials{}.alias)>::name()
            , affinity_traits<decltype(contact::contact_credentials{}.type)>::name())
        , fmt::format(CREATE_MEMBERS_TABLE
            , _members_table_name
            , affinity_traits<contact::contact_id>::name()
            , affinity_traits<contact::contact_id>::name())
        , fmt::format(CREATE_FOLLOWERS_TABLE
            , _followers_table_name
            , affinity_traits<contact::contact_id>::name()
            , affinity_traits<contact::contact_id>::name())

        , fmt::format(CREATE_CONTACTS_INDEX , _contacts_table_name)
        , fmt::format(CREATE_MEMBERS_INDEX  , _members_table_name)
        , fmt::format(CREATE_FOLLOWERS_INDEX, _followers_table_name)
    };

    debby::error err;
    auto success = _dbh->begin();

    if (success) {
        for (auto const & sql: sqls) {
            success = success && _dbh->query(sql, & err);
        }
    }

    if (success)
        _dbh->commit();
    else
        _dbh->rollback();

    if (!success) {
        on_failure(fmt::format(INIT_CONTACT_MANAGER_ERROR, err.what()));
        database_handle_t empty;
        _dbh.swap(empty);
    }
}

// namespace {
// std::string const COUNT_CONTACTS { "SELECT COUNT(1) as count FROM {}" };
// } // namespace
//
// std::size_t contact_manager::count_impl () const
// {
//     return _dbh->rows_count(_table_name);
// }
//
// namespace {
//
// std::string const INSERT_CONTACT {
//     "INSERT OR IGNORE INTO `{}` (`id`, `alias`, `type`)"
//     " VALUES (:id, :alias, :type)"
// };
//
// } // namespace
//
// int contact_manager::add_impl (contact::contact_credentials && c)
// {
//     contact::contact_credentials cc = std::move(c);
//     return add_impl(cc);
// }
//
// int contact_manager::add_impl (contact::contact_credentials const & c)
// {
//     invalidate_cache();
//
//     debby::error err;
//     auto stmt = _dbh->prepare(fmt::format(INSERT_CONTACT, _table_name), true, & err);
//     bool success = !!stmt;
//
//     success = success
//         && stmt.bind(":id", to_storage(c.id), false, & err)
//         && stmt.bind(":alias", to_storage(c.alias), false, & err)
//         && stmt.bind(":type", to_storage(c.type), & err);
//
//     if (success) {
//         auto res = stmt.exec(& err);
//
//         if (res.is_error())
//             success = false;
//     }
//
//     if (!success)
//         on_failure(fmt::format(ADD_CONTACT_ERROR, err.what()));
//
//     return success ? stmt.rows_affected() : -1;
// }
//
// namespace {
//
// std::string const UPDATE_CONTACT {
//     "UPDATE OR IGNORE `{}` SET `alias` = :alias, `type` = :type"
//     " WHERE `id` = :id"
// };
//
// } // namespace
//
// int contact_manager::update_impl (contact::contact_credentials const & c)
// {
//     invalidate_cache();
//
//     debby::error err;
//     auto stmt = _dbh->prepare(fmt::format(UPDATE_CONTACT, _table_name), true, & err);
//     bool success = !!stmt;
//
//     success = success
//         && stmt.bind(":alias", to_storage(c.alias), false, & err)
//         && stmt.bind(":type", to_storage(c.type), & err)
//         && stmt.bind(":id", to_storage(c.id), false, & err);
//
//     if (success) {
//         auto res = stmt.exec(& err);
//
//         if (res.is_error()) {
//             on_failure(fmt::format(UPDATE_CONTACT_ERROR, err.what()));
//             success = false;
//         }
//     } else {
//         on_failure(fmt::format(UPDATE_CONTACT_ERROR, err.what()));
//     }
//
//     return success ? stmt.rows_affected() : -1;
// }
//
// namespace {
//
// std::string const SELECT_CONTACT {
//     "SELECT `id`, `alias`, `type` FROM `{}` WHERE `id` = :id"
// };
//
// } // namespace
//
// bool contact_manager::fill_contact (result_t * res, contact::contact_credentials * c)
// {
//     input_record in {*res};
//
//     auto success = in["id"] >> c->id
//         && in["alias"]      >> c->alias
//         && in["type"]       >> c->type;
//
//     return success;
// }
//
// pfs::optional<contact::contact_credentials> contact_manager::get_impl (contact::contact_id id)
// {
//     // Check cache
//     if (!_cache.dirty) {
//         auto it = _cache.map.find(id);
//         if (it != _cache.map.end()) {
//             assert(it->second >= 0 && it->second < _cache.data.size());
//             return _cache.data[it->second];
//         }
//     }
//
//     debby::error err;
//     auto stmt = _dbh->prepare(fmt::format(SELECT_CONTACT, _table_name), true, & err);
//     bool success = !!stmt;
//
//     success = success && stmt.bind(":id", to_storage(id), false, & err);
//
//     if (success) {
//         auto res = stmt.exec(& err);
//
//         if (res.has_more()) {
//             contact::contact_credentials c;
//             success = fill_contact(& res, & c);
//
//             if (success)
//                 return std::move(c);
//         }
//
//         if (res.is_error())
//             success = false;
//     }
//
//     if (!success)
//         on_failure(fmt::format(LOAD_CONTACT_ERROR, err.what()));
//
//     return pfs::nullopt;
// }
//
// pfs::optional<contact::contact_credentials> contact_manager::get_impl (int offset)
// {
//     bool force_populate_cache = _cache.dirty;
//
//     // Out of range
//     if (offset < _cache.offset || offset >= _cache.offset + _cache.limit)
//         force_populate_cache = true;
//
//     // Populate cache if dirty
//     if (force_populate_cache) {
//         auto success = prefetch(offset, CACHE_WINDOW_SIZE);
//
//         if (!success)
//             return pfs::nullopt;
//     }
//
//     if (offset < _cache.offset || offset >= _cache.offset + _cache.limit)
//         return pfs::nullopt;
//
//     return _cache.data[offset - _cache.offset];
// }

namespace {

std::string const WIPE_TABLE {
    "DELETE FROM `{}`"
};

} // namespace

bool contact_manager::wipe_impl ()
{
    // FIXME
//     invalidate_cache();

    std::array<std::string, 3> sqls = {
          fmt::format(WIPE_TABLE, _contacts_table_name)
        , fmt::format(WIPE_TABLE, _members_table_name)
        , fmt::format(WIPE_TABLE, _followers_table_name)
    };

    debby::error err;
    auto success = _dbh->begin();

    if (success) {
        for (auto const & sql: sqls) {
            success = success && _dbh->query(sql, & err);
        }
    }

    if (success)
        _dbh->commit();
    else
        _dbh->rollback();

    if (!success) {
        on_failure(fmt::format(WIPE_ERROR, err.what()));
        database_handle_t empty;
        _dbh.swap(empty);
    }

    return success;
}

// namespace {
//
// std::string const SELECT_ALL_CONTACTS {
//     "SELECT `id`, `alias`, `type` FROM `{}`"
// };
//
// } // namespace
//
// bool contact_manager::all_of_impl (std::function<void(contact::contact_credentials const &)> f)
// {
//     debby::error err;
//     auto stmt = _dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _table_name), true, & err);
//     auto success = !!stmt;
//
//     if (success) {
//         auto res = stmt.exec(& err);
//
//         for (; success && res.has_more(); res.next()) {
//             contact::contact_credentials c;
//             success = fill_contact(& res, & c);
//
//             if (success)
//                 f(c);
//         }
//
//         if (res.is_error())
//             success = false;
//     }
//
//     if (!success)
//         on_failure(fmt::format(LOAD_ALL_CONTACTS_ERROR, err.what()));
//
//     return success;
// }
//
// namespace {
//
// std::string const SELECT_ROWS_RANGE {
//     "SELECT `id`, `alias`, `type` FROM `{}` LIMIT {} OFFSET {}"
// };
//
// } // namespace
//
// void contact_manager::invalidate_cache ()
// {
//     _cache.dirty = true;
// }
//
// int contact_manager::prefetch (int offset, int limit)
// {
//     bool prefetch_required = _cache.dirty
//         || offset < _cache.offset
//         || offset + limit > _cache.offset + _cache.limit;
//
//     if (!prefetch_required)
//         return true;
//
//     _cache.data.clear();
//     _cache.map.clear();
//     _cache.offset = offset;
//     _cache.limit = 0;
//     _cache.dirty = true;
//
//     debby::error err;
//     auto stmt = _dbh->prepare(fmt::format(SELECT_ROWS_RANGE, _table_name
//         , limit, offset), true, & err);
//     auto success = !!stmt;
//
//     if (success) {
//         auto res = stmt.exec(& err);
//
//         for (; success && res.has_more(); res.next()) {
//             contact::contact_credentials c;
//             success = fill_contact(& res, & c);
//
//             if (success) {
//                 _cache.data.push_back(std::move(c));
//                 _cache.map.emplace(c.id, _cache.data.size() - 1);
//                 _cache.limit++;
//             }
//         }
//
//         if (res.is_error())
//             success = false;
//     }
//
//     if (success) {
//         _cache.dirty = false;
//     } else {
//         on_failure(fmt::format(CACHE_CONTACTS_ERROR, err.what()));
//     }
//
//     return success;
// }

}}} // namespace chat::persistent_storage::sqlite3
