////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.16 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <array>
#include <cassert>

namespace chat {

using namespace debby::sqlite3;

namespace {
    constexpr std::size_t CACHE_WINDOW_SIZE = 100;

    std::string const DEFAULT_CONTACTS_TABLE_NAME  { "chat_contacts" };
    std::string const DEFAULT_MEMBERS_TABLE_NAME   { "chat_groups" };
    std::string const DEFAULT_FOLLOWERS_TABLE_NAME { "chat_channels" };

    std::string const INIT_CONTACT_MANAGER_ERROR { "initialization contact manager failure: {}" };
    std::string const WIPE_ERROR                 { "wipe contact data failure: {}" };
} // namespace

namespace {

std::string const CREATE_CONTACTS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`id` {} NOT NULL UNIQUE"
        ", `alias` {} NOT NULL"
        ", `avatar` {}"
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

namespace backend {
namespace sqlite3 {

contact_manager::rep_type
contact_manager::make (contact::person const & me
    , shared_db_handle dbh
    , error * perr)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.me  = me;
    rep.contacts_table_name  = DEFAULT_CONTACTS_TABLE_NAME;
    rep.members_table_name   = DEFAULT_MEMBERS_TABLE_NAME;
    rep.followers_table_name = DEFAULT_FOLLOWERS_TABLE_NAME;

    std::array<std::string, 6> sqls = {
          fmt::format(CREATE_CONTACTS_TABLE
            , rep.contacts_table_name
            , affinity_traits<contact::contact_id>::name()
            , affinity_traits<decltype(contact::contact{}.alias)>::name()
            , affinity_traits<decltype(contact::contact{}.avatar)>::name()
            , affinity_traits<decltype(contact::contact{}.type)>::name())
        , fmt::format(CREATE_MEMBERS_TABLE
            , rep.members_table_name
            , affinity_traits<contact::contact_id>::name()
            , affinity_traits<contact::contact_id>::name())
        , fmt::format(CREATE_FOLLOWERS_TABLE
            , rep.followers_table_name
            , affinity_traits<contact::contact_id>::name()
            , affinity_traits<contact::contact_id>::name())

        , fmt::format(CREATE_CONTACTS_INDEX , rep.contacts_table_name)
        , fmt::format(CREATE_MEMBERS_INDEX  , rep.members_table_name)
        , fmt::format(CREATE_FOLLOWERS_INDEX, rep.followers_table_name)
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

    if (success) {
        error err;

        rep.contacts = std::make_shared<contact_list_type>(
            contact_list_type::make(dbh, rep.contacts_table_name, & err));

        if (!err) {
            rep.groups = std::make_shared<group_list_type>(
                group_list_type::make(dbh
                    , rep.contacts_table_name
                    , rep.members_table_name
                    , rep.contacts
                    , & err));
        }

        if (err) {
            if (perr) *perr = err; CHAT__THROW(err);
            success = false;
        }
    }

    if (!success) {
        shared_db_handle empty;
        rep.dbh.swap(empty);
        error err {errc::storage_error, fmt::format(INIT_CONTACT_MANAGER_ERROR, storage_err.what())};
        if (perr) *perr = err; CHAT__THROW(err);
    }

    return rep;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::contact_manager

template <>
contact_manager<BACKEND>::contact_manager (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
contact_manager<BACKEND>::operator bool () const noexcept
{
    return !!_rep.dbh;
}

template <>
contact::contact
contact_manager<BACKEND>::my_contact () const
{
    return contact::contact {_rep.me.id
        , _rep.me.alias
        , _rep.me.avatar
        , contact::type_enum::person };
}

template <>
std::shared_ptr<contact_manager<BACKEND>::contact_list_type>
contact_manager<BACKEND>::contacts () const noexcept
{
    return _rep.contacts;
}

template <>
std::shared_ptr<contact_manager<BACKEND>::group_list_type>
contact_manager<BACKEND>::groups () const noexcept
{
    return _rep.groups;
}

template <>
bool
contact_manager<BACKEND>::contact_manager::wipe (error * perr)
{
    std::array<std::string, 3> tables = {
          _rep.contacts_table_name
        , _rep.members_table_name
        , _rep.followers_table_name
    };

    debby::error storage_err;
    auto success = _rep.dbh->begin();

    if (success) {
        for (auto const & t: tables) {
            success = success && _rep.dbh->clear(t, & storage_err);
        }
    }

    if (success)
        _rep.dbh->commit();
    else
        _rep.dbh->rollback();

    if (!success) {
        error err {errc::storage_error, fmt::format(WIPE_ERROR, storage_err.what())};
        if (perr) *perr = err; CHAT__THROW(err);
    }

    return success;
}

} // namespace chat
