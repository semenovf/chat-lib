////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.16 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/error.hpp"
#include "pfs/chat/backend/in_memory/contact_list.hpp"
#include "pfs/chat/backend/sqlite3/contact_list.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include "pfs/debby/backend/sqlite3/uuid_traits.hpp"
#include <array>

namespace chat {

using namespace debby::backend::sqlite3;

namespace backend {
namespace sqlite3 {

static std::string const DEFAULT_CONTACTS_TABLE_NAME  { "chat_contacts" };
static std::string const DEFAULT_MEMBERS_TABLE_NAME   { "chat_members" };
static std::string const DEFAULT_FOLLOWERS_TABLE_NAME { "chat_channels" };

static std::string const CREATE_CONTACTS_TABLE {
    //      ---- Optional TEMPORARY
    //      |
    //      v
    "CREATE {} TABLE IF NOT EXISTS `{}` ("
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
            , ""
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

static void fill_contact (backend::sqlite3::db_traits::result_type & result
    , contact::contact & c)
{
    result["id"]          >> c.contact_id;
    result["creator_id"]  >> c.creator_id;
    result["alias"]       >> c.alias;
    result["avatar"]      >> c.avatar;
    result["description"] >> c.description;
    result["type"]        >> c.type;
}

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
    static char const * SELECT_CONTACT = "SELECT `id`, `creator_id`, `alias`"
        ", `avatar`, `description`, `type` FROM `{}` WHERE `id` = :id";

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(SELECT_CONTACT, _rep.contacts_table_name));

        stmt.bind(":id", id);

        auto res = stmt.exec();

        if (res.has_more()) {
            contact::contact c;
            fill_contact(res, c);
            return c;
        }

        return contact::contact{};
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }
}

template <>
contact::contact
contact_manager<BACKEND>::at (int offset) const
{
    static char const * SELECT_CONTACT_AT = "SELECT `id`, `creator_id`"
        ", `alias`, `avatar`, `description`, `type` FROM `{}` LIMIT 1 OFFSET {}";

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(SELECT_CONTACT_AT
            , _rep.contacts_table_name, offset));

        auto res = stmt.exec();

        if (res.has_more()) {
            contact::contact c;
            fill_contact(res, c);
            return c;
        }

        return contact::contact{};
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }
}

template <>
contact_manager<BACKEND>::group_const_ref
contact_manager<BACKEND>::gref (contact::id group_id) const
{
    auto c = get(group_id);

    if (is_valid(c) && c.type == conversation_enum::group)
        return group_const_ref{group_id, this};

    return group_const_ref{};
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
    return _rep.dbh->rows_count(_rep.contacts_table_name);
}

template <>
std::size_t
contact_manager<BACKEND>::count (conversation_enum type) const
{
    static char const * COUNT_CONTACTS_BY_TYPE =
        "SELECT COUNT(1) as count FROM {} WHERE `type` = :type";

    //return _rep.contacts->count(type);
    std::size_t count = 0;
    auto stmt = _rep.dbh->prepare(fmt::format(COUNT_CONTACTS_BY_TYPE, _rep.contacts_table_name));

    stmt.bind(":type", to_storage(type));

    auto res = stmt.exec();

    if (res.has_more()) {
        count = res.get<std::size_t>(0);
        res.next();
    }

    return count;
}

static char const * INSERT_CONTACT =
    "INSERT OR IGNORE INTO `{}` (`id`, `creator_id`, `alias`, `avatar`"
    ", `description`, `type`)"
    " VALUES (:id, :creator_id, :alias, :avatar, :description, :type)";

template <>
bool
contact_manager<BACKEND>::add (contact::contact const & c)
{
    try {
        auto stmt = _rep.dbh->prepare(fmt::format(INSERT_CONTACT, _rep.contacts_table_name));

        stmt.bind(":id"         , to_storage(c.contact_id));
        stmt.bind(":creator_id" , to_storage(c.creator_id));
        stmt.bind(":alias"      , to_storage(c.alias));
        stmt.bind(":avatar"     , to_storage(c.avatar));
        stmt.bind(":description", to_storage(c.description));
        stmt.bind(":type"       , to_storage(c.type));

        auto res = stmt.exec();
        auto n = stmt.rows_affected();

        return n > 0;
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    return false;
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

    return this->add(c);
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

        if (this->add(c)) {
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
    static char const * UPDATE_CONTACT = "UPDATE OR IGNORE `{}` SET"
        " `alias` = :alias, `avatar` = :avatar, `description` = :description"
        " WHERE `id` = :id AND `type` = :type";

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(UPDATE_CONTACT, _rep.contacts_table_name));

        stmt.bind(":alias" , to_storage(c.alias));
        stmt.bind(":avatar", to_storage(c.avatar));
        stmt.bind(":description", to_storage(c.description));
        stmt.bind(":id", to_storage(c.contact_id));
        stmt.bind(":type", to_storage(c.type));

        auto res = stmt.exec();
        auto n = stmt.rows_affected();

        return n > 0;
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }
}

template <>
bool
contact_manager<BACKEND>::update (contact::person const & p)
{
    contact::contact c {
         p.contact_id
        , p.alias
        , p.avatar
        , p.description
        , p.contact_id
        , conversation_enum::person
    };

    return this->update(c);
}

template <>
bool
contact_manager<BACKEND>::update (contact::group const & g)
{
    contact::contact c {
          g.contact_id
        , g.alias
        , g.avatar
        , g.description
        , g.creator_id
        , conversation_enum::group
    };

    return this->update(c);
}

template <>
void
contact_manager<BACKEND>::remove (contact::id id)
{
    static char const * REMOVE_MEMBERSHIPS = "DELETE from `{}` WHERE `member_id` = :member_id";
    static char const * REMOVE_GROUP = "DELETE from `{}` WHERE `group_id` = :group_id";
    static char const * REMOVE_CONTACT = "DELETE from `{}` WHERE `id` = :id";

    auto stmt1 = _rep.dbh->prepare(fmt::format(REMOVE_MEMBERSHIPS, _rep.members_table_name));
    auto stmt2 = _rep.dbh->prepare(fmt::format(REMOVE_GROUP, _rep.members_table_name));
    auto stmt3 = _rep.dbh->prepare(fmt::format(REMOVE_CONTACT, _rep.contacts_table_name));

    stmt1.bind(":member_id", id);
    stmt2.bind(":group_id", id);
    stmt3.bind(":id", id);

    try {
        _rep.dbh->begin();

        for (auto * stmt: {& stmt1, & stmt2, & stmt3})
            auto res = stmt->exec();

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

        for (auto const & t: tables)
            _rep.dbh->clear(t);

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();
        throw error{errc::storage_error, ex.what()};
    }
}

static char const * SELECT_ALL_CONTACTS = "SELECT `id`, `creator_id`"
    ", `alias`, `avatar`, `description`, `type` FROM `{}`";


template <>
void
contact_manager<BACKEND>::for_each (std::function<void(contact::contact const &)> f) const
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _rep.contacts_table_name));
    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        contact::contact c;
        fill_contact(res, c);
        f(c);
    }
}

template <>
void
contact_manager<BACKEND>::for_each_movable (std::function<void(contact::contact &&)> f) const
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _rep.contacts_table_name));
    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        contact::contact c;
        fill_contact(res, c);
        f(std::move(c));
    }
}

template <>
void
contact_manager<BACKEND>::for_each_until (std::function<bool(contact::contact const &)> f) const
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _rep.contacts_table_name));
    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        contact::contact c;
        fill_contact(res, c);

        if (!f(c))
            break;
    }
}

template <>
void
contact_manager<BACKEND>::for_each_until_movable (std::function<bool(contact::contact &&)> f) const
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_ALL_CONTACTS, _rep.contacts_table_name));
    auto res = stmt.exec();

    for (; res.has_more(); res.next()) {
        contact::contact c;
        fill_contact(res, c);

        if (!f(std::move(c)))
            break;
    }
}

template <>
template <>
contact_list<backend::in_memory::contact_list>
contact_manager<BACKEND>::contacts<backend::in_memory::contact_list> (
    std::function<bool(contact::contact const &)> f) const
{
    backend::in_memory::contact_list::rep_type contact_list_rep;

    std::function<void(contact::contact &&)> ff = [& contact_list_rep, & f] (contact::contact && c) {
        if (f(c)) {
            contact_list_rep.map.emplace(c.contact_id, contact_list_rep.data.size());
            contact_list_rep.data.push_back(std::move(c));
        }
    };

    this->for_each_movable(std::move(ff));

    return contact_list<backend::in_memory::contact_list>(std::move(contact_list_rep));
}

template <>
template <>
contact_list<backend::sqlite3::contact_list>
contact_manager<BACKEND>::contacts<backend::sqlite3::contact_list> (
    std::function<bool(contact::contact const &)> f) const
{

    backend::sqlite3::contact_list::rep_type contact_list_rep;

    try {
        _rep.dbh->begin();

        contact_list_rep.table_name = "contact_list_" + to_string(pfs::generate_uuid());

        auto sql = fmt::format(backend::sqlite3::CREATE_CONTACTS_TABLE
            , "TEMPORARY"
            , contact_list_rep.table_name
            , affinity_traits<contact::id>::name()
            , affinity_traits<contact::id>::name()
            , affinity_traits<decltype(contact::contact{}.alias)>::name()
            , affinity_traits<decltype(contact::contact{}.avatar)>::name()
            , affinity_traits<decltype(contact::contact{}.description)>::name()
            , affinity_traits<decltype(contact::contact{}.type)>::name());

        _rep.dbh->query(sql);

        auto stmt = _rep.dbh->prepare(fmt::format(INSERT_CONTACT, contact_list_rep.table_name));

        std::function<void(contact::contact &&)> ff = [& stmt, & f] (contact::contact && c) {
            if (f(c)) {
                stmt.bind(":id"         , to_storage(c.contact_id));
                stmt.bind(":creator_id" , to_storage(c.creator_id));
                stmt.bind(":alias"      , to_storage(c.alias));
                stmt.bind(":avatar"     , to_storage(c.avatar));
                stmt.bind(":description", to_storage(c.description));
                stmt.bind(":type"       , to_storage(c.type));

                stmt.exec();
            }
        };

        this->for_each_movable(std::move(ff));

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();
        throw error {errc::storage_error, ex.what()};
    }

    contact_list_rep.dbh = _rep.dbh;
    return contact_list<backend::sqlite3::contact_list>(std::move(contact_list_rep));
}

} // namespace chat
