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
        ", `description` {}"
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
            , affinity_traits<decltype(contact::contact{}.description)>::name()
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
        , _rep.me.description
        , contact::type_enum::person };
}

template <>
std::size_t
contact_manager<BACKEND>::count () const
{
    return _rep.contacts->count();
}

template <>
std::size_t
contact_manager<BACKEND>::count (contact::type_enum type) const
{
    return _rep.contacts->count(type);
}

template <>
int
contact_manager<BACKEND>::add (contact::contact const & c, error * perr)
{
    return _rep.contacts->add(c, perr);
}

template <>
int
contact_manager<BACKEND>::batch_add (std::function<bool()> has_next
    , std::function<contact::contact()> next
    , error * perr)
{
    int counter = 0;
    bool success = _rep.dbh->begin();

    if (success) {
        error err;

        while (!err && has_next()) {
            auto n = add(next(), & err);
            counter += n > 0 ? 1 : 0;
        }

        if (err) {
            if (perr) *perr = err; else CHAT__THROW(err);
            success = false;
        }
    }

    if (success) {
        _rep.dbh->commit();
    } else {
        _rep.dbh->rollback();
        counter = -1;
    }

    return counter;
}

template <>
int
contact_manager<BACKEND>::update (contact::contact const & c, error * perr)
{
    return _rep.contacts->update(c, perr);
}

template <>
contact::contact
contact_manager<BACKEND>::get (contact::contact_id id, error * perr) const
{
    return _rep.contacts->get(id, perr);
}

template <>
contact::contact
contact_manager<BACKEND>::get (int offset, error * perr) const
{
    return _rep.contacts->get(offset, perr);
}

namespace {
std::string const REMOVE_CONTACT {
    "DELETE from `{}` WHERE `id` = :id"
};

std::string const REMOVE_MEMBERSHIPS {
    "DELETE from `{}` WHERE `member_id` = :member_id"
};

std::string const REMOVE_GROUP {
    "DELETE from `{}` WHERE `group_id` = :group_id"
};

} // namespace

template <>
bool
contact_manager<BACKEND>::remove (contact::contact_id id, error * perr)
{
    debby::error storage_err;
    auto stmt1 = _rep.dbh->prepare(fmt::format(REMOVE_CONTACT, _rep.contacts_table_name)
        , true, & storage_err);
    auto stmt2 = _rep.dbh->prepare(fmt::format(REMOVE_MEMBERSHIPS, _rep.members_table_name)
        , true, & storage_err);
    auto stmt3 = _rep.dbh->prepare(fmt::format(REMOVE_GROUP, _rep.members_table_name)
        , true, & storage_err);
    bool success = !!stmt1 && !!stmt2 && !!stmt3;

    success = success
        && stmt1.bind(":id", to_storage(id), false, & storage_err)
        && stmt2.bind(":member_id", to_storage(id), false, & storage_err)
        && stmt2.bind(":group_id", to_storage(id), false, & storage_err);

    if (success) {
        success = _rep.dbh->begin();

        for (auto * stmt: {& stmt1, & stmt2, & stmt3}) {
            auto res = stmt->exec(& storage_err);

            if (res.is_error()) {
                success = false;
                break;
            }
        }

        if (success)
            _rep.dbh->commit();
        else
            _rep.dbh->rollback();
    }

    if (!success) {
        error err{errc::storage_error
            , fmt::format("remove contact failure: #{}"
                , to_string(id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    return success;
}

template <>
bool
contact_manager<BACKEND>::wipe (error * perr)
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

namespace {
std::string const SELECT_ALL_CONTACTS {
    "SELECT `id`, `alias`, `avatar`, `description`, `type` FROM `{}`"
};
} // namespace

template <>
bool
contact_manager<BACKEND>::for_each (std::function<void(contact::contact const &)> f
    , error * perr)
{
    return _rep.contacts->for_each(f, perr);
}

} // namespace chat
