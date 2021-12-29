////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/persistent_storage/sqlite3/contact_manager.hpp"
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
        ", `member_id` {} NOT NULL)"
};

std::string const CREATE_FOLLOWERS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`channel_id` {} NOT NULL"
        ", `follower_id` {} NOT NULL)"
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
    , _contacts(dbh, _contacts_table_name, on_failure)
    , _groups(dbh, _contacts, _contacts_table_name, _members_table_name, on_failure)
{
    std::array<std::string, 6> sqls = {
          fmt::format(CREATE_CONTACTS_TABLE
            , _contacts_table_name
            , affinity_traits<contact::contact_id>::name()
            , affinity_traits<decltype(contact::contact{}.alias)>::name()
            , affinity_traits<decltype(contact::contact{}.type)>::name())
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

namespace {

std::string const WIPE_TABLE {
    "DELETE FROM `{}`"
};

} // namespace

bool contact_manager::wipe_impl ()
{
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

}}} // namespace chat::persistent_storage::sqlite3