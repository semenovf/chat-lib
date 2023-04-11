////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.16 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "conversation_enum_traits.hpp"
#include "pfs/assert.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/error.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include "pfs/debby/backend/sqlite3/uuid_traits.hpp"
#include <array>
#include <cassert>

namespace chat {

using namespace debby::backend::sqlite3;

namespace backend {
namespace sqlite3 {

static std::string const DEFAULT_CONTACTS_TABLE_NAME  { "chat_contacts" };
static std::string const DEFAULT_MEMBERS_TABLE_NAME   { "chat_members" };
static std::string const DEFAULT_FOLLOWERS_TABLE_NAME { "chat_channels" };

static std::string const CREATE_CONTACTS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`id` {} NOT NULL UNIQUE"
        ", `creator_id` {} NOT NULL"
        ", `alias` {} NOT NULL"
        ", `avatar` {}"
        ", `description` {}"
        ", `type` {} NOT NULL"
        ", PRIMARY KEY(`id`)) WITHOUT ROWID"
};

static std::string const CREATE_MEMBERS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`group_id` {} NOT NULL"
        ", `member_id` {} NOT NULL)"
};

static std::string const CREATE_FOLLOWERS_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`channel_id` {} NOT NULL"
        ", `follower_id` {} NOT NULL)"
};

static std::string const CREATE_CONTACTS_INDEX {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{0}_index` ON `{0}` (`id`)"
};

static std::string const CREATE_MEMBERS_INDEX {
    "CREATE INDEX IF NOT EXISTS `{0}_index` ON `{0}` (`group_id`)"
};

// Preventing duplicate pairs of group_id:member_id
static std::string const CREATE_MEMBERS_UNIQUE_INDEX {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{0}_unique_index` ON `{0}` (`group_id`, `member_id`)"
};

static std::string const CREATE_FOLLOWERS_INDEX {
    "CREATE INDEX IF NOT EXISTS `{0}_index` ON `{0}` (`channel_id`)"
};

contact_manager::rep_type
contact_manager::make (contact::person const & me, shared_db_handle dbh)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.me  = me;
    rep.contacts_table_name  = DEFAULT_CONTACTS_TABLE_NAME;
    rep.members_table_name   = DEFAULT_MEMBERS_TABLE_NAME;
    rep.followers_table_name = DEFAULT_FOLLOWERS_TABLE_NAME;

    std::array<std::string, 7> sqls = {
          fmt::format(CREATE_CONTACTS_TABLE
            , rep.contacts_table_name
            , affinity_traits<contact::id>::name()
            , affinity_traits<contact::id>::name()
            , affinity_traits<decltype(contact::contact{}.alias)>::name()
            , affinity_traits<decltype(contact::contact{}.avatar)>::name()
            , affinity_traits<decltype(contact::contact{}.description)>::name()
            , affinity_traits<decltype(contact::contact{}.type)>::name())
        , fmt::format(CREATE_MEMBERS_TABLE
            , rep.members_table_name
            , affinity_traits<contact::id>::name()
            , affinity_traits<contact::id>::name())
        , fmt::format(CREATE_FOLLOWERS_TABLE
            , rep.followers_table_name
            , affinity_traits<contact::id>::name()
            , affinity_traits<contact::id>::name())
        , fmt::format(CREATE_CONTACTS_INDEX , rep.contacts_table_name)
        , fmt::format(CREATE_MEMBERS_INDEX  , rep.members_table_name)
        , fmt::format(CREATE_MEMBERS_UNIQUE_INDEX, rep.members_table_name)
        , fmt::format(CREATE_FOLLOWERS_INDEX, rep.followers_table_name)
    };

    try {
        rep.dbh->begin();

        for (auto const & sql: sqls)
            rep.dbh->query(sql);

        rep.contacts = std::make_shared<contact_list_type>(
            contact_list_type::make(dbh, rep.contacts_table_name));

        rep.dbh->commit();
    } catch (debby::error ex) {
        rep.dbh->rollback();

        shared_db_handle empty;
        rep.dbh.swap(empty);
        throw error{errc::storage_error, ex.what()};
    }

    return rep;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::contact_manager;

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
bool
contact_manager<BACKEND>::transaction (std::function<bool()> op) noexcept
{
    bool success = true;

    try {
        _rep.dbh->begin();
        success = op();
        _rep.dbh->commit();
    } catch (...) {
        _rep.dbh->rollback();
        return false;
    }

    return success;
}

template <>
contact::contact
contact_manager<BACKEND>::get (contact::id id) const
{
    return _rep.contacts->get(id);
}

template <>
contact::contact
contact_manager<BACKEND>::get (int offset) const
{
    return _rep.contacts->get(offset);
}

template <>
contact_manager<BACKEND>::group_ref
contact_manager<BACKEND>::gref (contact::id group_id)
{
    auto c = get(group_id);

    if (is_valid(c) && c.type == conversation_enum::group)
        return group_ref{group_id, this};

    return group_ref{};
}

template <>
contact::person
contact_manager<BACKEND>::my_contact () const
{
    return contact::person {
          _rep.me.contact_id
        , _rep.me.alias
        , _rep.me.avatar
        , _rep.me.description
    };
}

template <>
void
contact_manager<BACKEND>::change_my_alias (std::string const & alias)
{
    if (!alias.empty())
        _rep.me.alias = alias;
}

template <>
void
contact_manager<BACKEND>::change_my_avatar (std::string const & avatar)
{
    _rep.me.avatar = avatar;
}

template <>
void
contact_manager<BACKEND>::change_my_desc (std::string const & desc)
{
    _rep.me.description = desc;
}

template <>
std::size_t
contact_manager<BACKEND>::count () const
{
    return _rep.contacts->count();
}

template <>
std::size_t
contact_manager<BACKEND>::count (conversation_enum type) const
{
    return _rep.contacts->count(type);
}

template <>
bool
contact_manager<BACKEND>::add (contact::person const & p)
{
    contact::contact c {
          p.contact_id
        , p.alias
        , p.avatar
        , p.description
        , p.contact_id
        , chat::conversation_enum::person
    };

    return _rep.contacts->add(c);
}

template <>
bool
contact_manager<BACKEND>::add (contact::group const & g)
{
    return transaction([this, & g/*, creator_id*/] {
        contact::contact c {
              g.contact_id
            , g.alias
            , g.avatar
            , g.description
            , g.creator_id
            , chat::conversation_enum::group};

        if (_rep.contacts->add(c)) {
            auto gr = this->gref(g.contact_id);

            if (!gr)
                return false;

            gr.add_member_unchecked(g.creator_id);

            return true;
        }

        return false;
    });
}

template <>
bool
contact_manager<BACKEND>::update (contact::contact const & c)
{
    return _rep.contacts->update(c);
}

namespace {
std::string const REMOVE_MEMBERSHIPS {
    "DELETE from `{}` WHERE `member_id` = :member_id"
};

std::string const REMOVE_GROUP {
    "DELETE from `{}` WHERE `group_id` = :group_id"
};

} // namespace

template <>
void
contact_manager<BACKEND>::remove (contact::id id)
{
    auto stmt1 = _rep.dbh->prepare(fmt::format(REMOVE_MEMBERSHIPS, _rep.members_table_name));
    auto stmt2 = _rep.dbh->prepare(fmt::format(REMOVE_GROUP, _rep.members_table_name));

    stmt1.bind(":member_id", id);
    stmt2.bind(":group_id", id);

    try {
        _rep.dbh->begin();

        _rep.contacts->remove(id);

        for (auto * stmt: {& stmt1, & stmt2}) {
            auto res = stmt->exec();
        }

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();
        throw error{errc::storage_error, ex.what()};
    }
}

template <>
void
contact_manager<BACKEND>::wipe ()
{
    std::array<std::string, 3> tables = {
          _rep.contacts_table_name
        , _rep.members_table_name
        , _rep.followers_table_name
    };

    try {
        _rep.dbh->begin();

        for (auto const & t: tables) {
            _rep.dbh->clear(t);
        }

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();
        throw error{errc::storage_error, ex.what()};
    }
}

template <>
void
contact_manager<BACKEND>::for_each (std::function<void(contact::contact const &)> f)
{
    _rep.contacts->for_each(f);
}

template <>
void
contact_manager<BACKEND>::for_each_until (std::function<bool(contact::contact const &)> f)
{
    _rep.contacts->for_each_until(f);
}

} // namespace chat
