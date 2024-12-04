////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.16 Refactored totally.
//      2024.11.24 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "contact_list_impl.hpp"
#include "contact_manager_impl.hpp"
#include "column_affinity.hpp"
#include "chat/contact.hpp"
#include "chat/contact_manager.hpp"
#include "chat/in_memory.hpp"
#include <pfs/i18n.hpp>
#include <pfs/debby/data_definition.hpp>
#include <string>
#include <type_traits>

namespace chat {

using data_definition_t = debby::data_definition<debby::backend_enum::sqlite3>;
using contact_manager_t = contact_manager<storage::sqlite3>;

namespace storage {

sqlite3::contact_manager::contact_manager (contact::person const & my_contact, relational_database_t & db)
    : my_contact_id(my_contact.contact_id)
{
    auto me = data_definition_t::create_table(my_contact_table_name);
    me.add_column<decltype(contact::person::contact_id)>("id").unique();
    me.add_column<decltype(contact::person::alias)>("alias");
    me.add_column<decltype(contact::person::avatar)>("avatar").nullable();
    me.add_column<decltype(contact::person::description)>("description").nullable();
    me.add_column<decltype(contact::person::extra)>("extra").nullable();

    auto contacts = data_definition_t::create_table(contacts_table_name);
    contacts.add_column<decltype(contact::contact::contact_id)>("id").primary_key().unique();
    contacts.add_column<decltype(contact::contact::creator_id)>("creator_id");
    contacts.add_column<decltype(contact::contact::alias)>("alias");
    contacts.add_column<decltype(contact::contact::avatar)>("avatar").nullable();
    contacts.add_column<decltype(contact::contact::description)>("description").nullable();
    contacts.add_column<decltype(contact::contact::extra)>("extra").nullable();
    contacts.add_column<decltype(contact::contact::type)>("type");
    contacts.constraint("WITHOUT ROWID");

    auto members = data_definition_t::create_table(members_table_name);
    members.add_column<contact::id>("group_id");
    members.add_column<contact::id>("member_id");

    auto channels = data_definition_t::create_table(followers_table_name);
    channels.add_column<contact::id>("channel_id");
    channels.add_column<contact::id>("follower_id");

    auto contacts_uindex = data_definition_t::create_index(contacts_table_name + "_uindex");
    contacts_uindex.unique().on(contacts_table_name).add_column("id");

    auto members_index = data_definition_t::create_index(members_table_name + "_index");
    members_index.on(members_table_name).add_column("group_id");

    // Preventing duplicate pairs of group_id::member_id
    auto members_uindex = data_definition_t::create_index(members_table_name + "_uindex");
    members_uindex.unique().on(members_table_name).add_column("group_id").add_column("member_id");

    auto followers_index = data_definition_t::create_index(followers_table_name + "_index");
    followers_index.on(followers_table_name).add_column("channel_id");

    std::array<std::string, 8> sqls = {
          me.build()
        , contacts.build()
        , members.build()
        , channels.build()
        , contacts_uindex.build()
        , members_index.build()
        , members_uindex.build()
        , followers_index.build()
    };

    auto failure = db.transaction([& sqls, & db] () {
        debby::error err;

        for (auto const & sql: sqls) {
            db.query(sql, & err);

            if (err)
                return pfs::make_optional(std::string{err.what()});
        }

        return pfs::optional<std::string>{};
    });

    if (failure)
        throw error {errc::storage_error, failure.value()};

    if (my_contact.contact_id == contact::id{}) {
        // Expected self contact already written, extract id
        debby::error err;
        auto sql = fmt::format("SELECT id FROM \"{}\"", my_contact_table_name);
        auto res = db.exec(sql, & err);

        if (!err) {
            if (res.has_more()) {
                my_contact_id = res.get_or(0, contact::id{});

                if (my_contact_id == contact::id{}) {
                    throw error {
                          errc::inconsistent_data
                        , tr::_("bad self contact identifier stored")
                    };
                }
            } else {
                throw error {
                      errc::contact_not_found
                    , tr::_("self contact credentials not found in the storage")
                };
            }
        }
    } else {
        debby::error err;
        auto sql = fmt::format("INSERT OR REPLACE INTO \"{}\" (id, alias, avatar, description, extra)"
            " VALUES (\"{}\", \"{}\", \"{}\", \"{}\", \"{}\")"
        , my_contact_table_name
        , to_string(my_contact.contact_id)
        , my_contact.alias
        , my_contact.avatar
        , my_contact.description
        , my_contact.extra);

        db.query(sql, & err);

        if (err) {
            throw error {
                  errc::storage_error
                , tr::f_("store self contact credentials failure: {}", err.what())
            };
        }
    }

    pdb = & db;
}

sqlite3::contact_manager *
sqlite3::make_contact_manager (contact::person const & my_contact, relational_database_t & db)
{
    return new sqlite3::contact_manager(my_contact, db);
}

sqlite3::contact_manager *
sqlite3::make_contact_manager (relational_database_t & db)
{
    return new sqlite3::contact_manager(contact::person{}, db);
}

} // namespace storage

template <>
contact_manager_t::contact_manager (rep * d) noexcept
    : _d(d)
{}

template <> contact_manager_t::contact_manager (contact_manager && other) noexcept = default;
template <> contact_manager_t & contact_manager_t::operator = (contact_manager && other) noexcept = default;
template <> contact_manager_t::~contact_manager () = default;

template <>
contact_manager_t::operator bool () const noexcept
{
    return !!_d && _d->pdb != nullptr;
}

template <>
pfs::optional<std::string>
contact_manager_t::transaction (std::function<pfs::optional<std::string>()> op)
{
    return _d->pdb->transaction([& op] { return op(); });
}

template <>
contact::person contact_manager_t::my_contact () const
{
    static char const * SELECT_MY_CONTACT = "SELECT id, alias, avatar, description, extra"
        " FROM \"{}\" WHERE id = :id";

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_MY_CONTACT, _d->my_contact_table_name), & err);

    if (!err) {
        stmt.bind(":id", _d->my_contact_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more()) {
                    contact::person p;
                    p.contact_id  = res.get_or("id", contact::id{});
                    p.alias       = res.get_or("alias", std::string{});
                    p.avatar      = res.get_or("avatar", std::string{});
                    p.description = res.get_or("description", std::string{});
                    p.extra       = res.get_or("extra", std::string{});

                    return p;
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return contact::person{};
}

template <>
contact::contact contact_manager_t::get (contact::id id) const
{
    static char const * SELECT_CONTACT = "SELECT id, creator_id, alias"
        ", avatar, description, extra, type FROM \"{}\" WHERE id = :id";

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_CONTACT, _d->contacts_table_name), & err);

    if (!err) {
        stmt.bind(":id", id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more()) {
                    contact::contact c;
                    storage::sqlite3::contact_list::fill_contact(res, c);
                    return c;
                }
            }
        }
    }

    if (err)
        throw error{errc::storage_error, err.what()};

    return contact::contact{};
}

template <>
contact_manager_t::group_const_ref contact_manager_t::gref (contact::id group_id) const
{
    auto c = get(group_id);

    if (is_valid(c) && c.type == chat_enum::group)
        return group_const_ref{group_id, this};

    return group_const_ref{};
}

template <>
contact_manager_t::group_ref contact_manager_t::gref (contact::id group_id)
{
    auto c = get(group_id);

    if (is_valid(c) && c.type == chat_enum::group)
        return group_ref{group_id, this};

    return group_ref{};
}

static char const * INSERT_CONTACT =
    "INSERT OR IGNORE INTO \"{}\" (id, creator_id, alias, avatar, description, extra, type)"
    " VALUES (:id, :creator_id, :alias, :avatar, :description, :extra, :type)";

template <>
bool contact_manager_t::add (contact::contact && c)
{
    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(INSERT_CONTACT, _d->contacts_table_name), & err);

    if (!err) {
        stmt.bind(":id", c.contact_id, & err)
            && stmt.bind(":creator_id" , c.creator_id, & err)
            && stmt.bind(":alias"      , std::move(c.alias), & err)
            && stmt.bind(":avatar"     , std::move(c.avatar), & err)
            && stmt.bind(":description", std::move(c.description), & err)
            && stmt.bind(":extra"      , std::move(c.extra), & err)
            && stmt.bind(":type"       , static_cast<std::underlying_type_t<decltype(c.type)>>(c.type), & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                auto n = res.rows_affected();
                return n > 0;
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return false;
}

template <>
bool contact_manager_t::add (contact::person && p)
{
    contact::contact c {
          p.contact_id
        , std::move(p.alias)
        , std::move(p.avatar)
        , std::move(p.description)
        , std::move(p.extra)
        , p.contact_id
        , chat::chat_enum::person
    };

    return this->add(std::move(c));
}

template <>
bool contact_manager_t::add (contact::group && g)
{
    contact::contact c {
          g.contact_id
        , std::move(g.alias)
        , std::move(g.avatar)
        , std::move(g.description)
        , std::move(g.extra)
        , g.creator_id
        , chat::chat_enum::group
    };

    auto failure = transaction([this, & c] {
        auto group_id = c.contact_id;
        auto creator_id = c.creator_id;

        if (this->add(std::move(c))) {
            auto gr = this->gref(group_id);

            if (!gr) {
                throw error {
                      pfs::make_error_code(pfs::errc::unexpected_error)
                    , tr::f_("group not found after added: {}", group_id)
                };
            }

            gr.add_member_unchecked(creator_id);

            return pfs::optional<std::string>{};
        }

        return pfs::optional<std::string>{"group not added"};
    });

    if (failure)
        throw error {errc::storage_error, failure.value()};

    return !failure;
}

template <>
std::size_t contact_manager_t::count () const
{
    debby::error err;
    auto n = _d->pdb->rows_count(_d->contacts_table_name, & err);

    if (err)
        throw error{errc::storage_error, err.what()};

    return n;
}

template <>
std::size_t contact_manager_t::count (chat_enum type) const
{
    static char const * COUNT_CONTACTS_BY_TYPE = "SELECT COUNT(1) as count FROM \"{}\" WHERE type = :type";

    std::size_t count = 0;
    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(COUNT_CONTACTS_BY_TYPE, _d->contacts_table_name), & err);

    if (!err) {
        stmt.bind(":type", type, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more()) {
                    count = res.get_or<std::size_t>(0, 0);
                    res.next();
                }
            }
        }
    }

    if (err)
        throw error{errc::storage_error, err.what()};

    return count;
}

template <>
void contact_manager_t::clear ()
{
    std::array<std::string, 3> tables = {
          _d->contacts_table_name
        , _d->members_table_name
        , _d->followers_table_name
    };

    auto failure = _d->pdb->transaction([this, & tables] {
        debby::error err;

        for (auto const & t: tables) {
            _d->pdb->clear(t, & err);

            if (err)
                return pfs::make_optional(std::string{err.what()});
        }

        return pfs::optional<std::string>{};
    });

    if (failure)
        throw error{errc::storage_error, failure.value()};
}

template <>
contact::contact contact_manager_t::at (int offset) const
{
    static char const * SELECT_CONTACT_AT = "SELECT id, creator_id"
        ", alias, avatar, description, extra, type FROM \"{}\" LIMIT 1 OFFSET {}";

    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_CONTACT_AT, _d->contacts_table_name, offset));

    if (err)
        throw error{errc::storage_error, err.what()};

    if (res.has_more()) {
        contact::contact c;
        storage::sqlite3::contact_list::fill_contact(res, c);
        return c;
    }

    return contact::contact{};
}

static char const * SELECT_ALL_CONTACTS = "SELECT id, creator_id, alias, avatar"
    ", description, extra, type FROM \"{}\"";

template <>
void contact_manager_t::for_each (std::function<void(contact::contact const &)> f) const
{
    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ALL_CONTACTS, _d->contacts_table_name), & err);

    if (!err) {
        for (; res.has_more(); res.next()) {
            contact::contact c;
            storage::sqlite3::contact_list::fill_contact(res, c);
            f(c);
        }
    } else {
        throw error {errc::storage_error, err.what()};
    }
}

template <>
void contact_manager_t::for_each_movable (std::function<void(contact::contact &&)> f) const
{
    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ALL_CONTACTS, _d->contacts_table_name));

    if (!err) {
        for (; res.has_more(); res.next()) {
            contact::contact c;
            storage::sqlite3::contact_list::fill_contact(res, c);
            f(std::move(c));
        }
    } else {
        throw error {errc::storage_error, err.what()};
    }
}

template <>
void contact_manager_t::for_each_until (std::function<bool(contact::contact const &)> f) const
{
    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ALL_CONTACTS, _d->contacts_table_name), & err);

    if (!err) {
        for (; res.has_more(); res.next()) {
            contact::contact c;
            storage::sqlite3::contact_list::fill_contact(res, c);

            if (!f(c))
                break;
        }
    } else {
        throw error {errc::storage_error, err.what()};
    }
}

template <>
void contact_manager_t::for_each_until_movable (std::function<bool(contact::contact &&)> f) const
{
    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ALL_CONTACTS, _d->contacts_table_name), & err);

    if (!err) {
        for (; res.has_more(); res.next()) {
            contact::contact c;
            storage::sqlite3::contact_list::fill_contact(res, c);

            if (!f(std::move(c)))
                break;
        }
    } else {
        throw error {errc::storage_error, err.what()};
    }
}

template <>
template <>
contact_list<storage::in_memory>
contact_manager_t::contacts<contact_list<storage::in_memory>> (
    std::function<bool(contact::contact const &)> f) const
{
    contact_list<storage::in_memory> result;

    this->for_each_movable([& result, & f] (contact::contact && c) {
        if (f(c))
            result.add(std::move(c));
    });

    return result;
}

template <>
template <>
contact_list<storage::sqlite3>
contact_manager_t::contacts<contact_list<storage::sqlite3>> (
    std::function<bool(contact::contact const &)> f) const
{
    auto table_name = "contact_list_" + to_string(pfs::generate_uuid());

    // Same as `contacts` table in constructor of `sqlite3::contact_manager`, excluding
    // TEMPORARY
    auto contacts = data_definition_t::create_table(table_name);
    contacts.temporary();
    contacts.add_column<decltype(contact::contact::contact_id)>("id").primary_key().unique();
    contacts.add_column<decltype(contact::contact::creator_id)>("creator_id");
    contacts.add_column<decltype(contact::contact::alias)>("alias");
    contacts.add_column<decltype(contact::contact::avatar)>("avatar").nullable();
    contacts.add_column<decltype(contact::contact::description)>("description").nullable();
    contacts.add_column<decltype(contact::contact::extra)>("extra").nullable();
    contacts.add_column<decltype(contact::contact::type)>("type");
    contacts.constraint("WITHOUT ROWID");

    debby::error err;
    _d->pdb->query(contacts.build(), & err);

    if (err)
        throw error {errc::storage_error, err.what()};

    auto stmt = _d->pdb->prepare_cached(fmt::format(INSERT_CONTACT, table_name), & err);

    if (err)
        throw error {errc::storage_error, err.what()};

    auto *pstmt = & stmt;

    auto failure = _d->pdb->transaction([this, pstmt, & f] {
        std::function<void(contact::contact &&)> ff = [pstmt, & f] (contact::contact && c) {
            if (f(c)) {
                debby::error err;
                pstmt->reset(& err);

                if (err)
                    throw error {errc::storage_error, err.what()};

                auto success = pstmt->bind(":id", c.contact_id, & err)
                    && pstmt->bind(":creator_id" , c.creator_id, & err)
                    && pstmt->bind(":alias"      , std::move(c.alias), & err)
                    && pstmt->bind(":avatar"     , std::move(c.avatar), & err)
                    && pstmt->bind(":description", std::move(c.description), & err)
                    && pstmt->bind(":extra"      , std::move(c.extra), & err)
                    && pstmt->bind(":type"       , static_cast<std::underlying_type_t<decltype(c.type)>>(c.type), & err);

                if (success)
                    pstmt->exec(& err);

                if (err)
                    throw error {errc::storage_error, err.what()};
            }
        };

        this->for_each_movable(std::move(ff));

        return pfs::optional<std::string>{};
    });

    if (failure)
        throw error{errc::storage_error, failure.value()};

    auto * d = storage::sqlite3::make_contact_list(table_name, *_d->pdb);
    return contact_list<storage::sqlite3>{d};
}

template <>
bool contact_manager_t::update (contact::contact && c)
{
    static char const * UPDATE_CONTACT = "UPDATE OR IGNORE \"{}\" SET"
        " alias = :alias, avatar = :avatar, description = :description, extra = :extra"
        " WHERE id = :id AND type = :type";

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(UPDATE_CONTACT, _d->contacts_table_name), & err);

    if (err)
        throw error {errc::storage_error, err.what()};

    auto success = stmt.bind(":alias", std::move(c.alias), & err)
        && stmt.bind(":avatar", std::move(c.avatar), & err)
        && stmt.bind(":description", std::move(c.description), & err)
        && stmt.bind(":extra", std::move(c.extra), & err)
        && stmt.bind(":id", c.contact_id, & err)
        && stmt.bind(":type", static_cast<std::underlying_type_t<decltype(c.type)>>(c.type), & err);

    if (!success)
        throw error {errc::storage_error, err.what()};

    auto res = stmt.exec(& err);

    if (err)
        throw error {errc::storage_error, err.what()};

    auto n = res.rows_affected();
    return n > 0;
}

template <>
bool contact_manager_t::update (contact::person && p)
{
    contact::contact c {
          p.contact_id
        , std::move(p.alias)
        , std::move(p.avatar)
        , std::move(p.description)
        , std::move(p.extra)
        , p.contact_id
        , chat_enum::person
    };

    return this->update(std::move(c));
}

template <>
bool contact_manager_t::update (contact::group && g)
{
    contact::contact c {
          g.contact_id
        , std::move(g.alias)
        , std::move(g.avatar)
        , std::move(g.description)
        , std::move(g.extra)
        , g.creator_id
        , chat_enum::group
    };

    return this->update(std::move(c));
}

template <>
void contact_manager_t::remove (contact::id id)
{
    static char const * REMOVE_MEMBERSHIPS = "DELETE FROM \"{}\" WHERE member_id = :member_id";
    static char const * REMOVE_GROUP = "DELETE FROM \"{}\" WHERE group_id = :group_id";
    static char const * REMOVE_CONTACT = "DELETE FROM \"{}\" WHERE id = :id";

    debby::error err;
    auto stmt1 = _d->pdb->prepare_cached(fmt::format(REMOVE_MEMBERSHIPS, _d->members_table_name), & err);

    if (err)
        throw error{errc::storage_error, err.what()};

    auto stmt2 = _d->pdb->prepare_cached(fmt::format(REMOVE_GROUP, _d->members_table_name), & err);

    if (err)
        throw error{errc::storage_error, err.what()};

    auto stmt3 = _d->pdb->prepare_cached(fmt::format(REMOVE_CONTACT, _d->contacts_table_name), & err);

    if (err)
        throw error{errc::storage_error, err.what()};

    stmt1.bind(":member_id", id, & err)
        && stmt2.bind(":group_id", id, & err)
        && stmt3.bind(":id", id, & err);

    if (err)
        throw error{errc::storage_error, err.what()};

    auto opterr = contact_manager_t::transaction ([& stmt1, & stmt2, & stmt3] () {
        debby::error err;

        for (auto * stmt: {& stmt1, & stmt2, & stmt3}) {
            auto res = stmt->exec(& err);

            if (err)
                return pfs::make_optional(std::string{err.what()});
        }

        return pfs::optional<std::string>{};
    });

    if (opterr)
        throw error{errc::storage_error, *opterr};
}

template <>
void contact_manager_t::change_my_alias (std::string && alias)
{
    static char const * UPDATE_MY_ALIAS = "UPDATE \"{}\" SET alias = :alias";

    if (alias.empty())
        return;

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(UPDATE_MY_ALIAS, _d->my_contact_table_name), & err);

    if (!err) {
        stmt.bind(":alias", std::move(alias), & err);

        if (!err)
           stmt.exec(& err);
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

template <>
void contact_manager_t::change_my_avatar (std::string && avatar)
{
    static char const * UPDATE_MY_AVATAR = "UPDATE \"{}\" SET avatar = :avatar";

    if (avatar.empty())
        return;

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(UPDATE_MY_AVATAR, _d->my_contact_table_name), & err);

    if (!err) {
        stmt.bind(":avatar", std::move(avatar), & err);

        if (!err)
           stmt.exec(& err);
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

template <>
void contact_manager_t::change_my_desc (std::string && desc)
{
    static char const * UPDATE_MY_DESC = "UPDATE \"{}\" SET description = :description";

    if (desc.empty())
        return;

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(UPDATE_MY_DESC, _d->my_contact_table_name), & err);

    if (!err) {
        stmt.bind(":description", std::move(desc), & err);

        if (!err)
           stmt.exec(& err);
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

} // namespace chat
