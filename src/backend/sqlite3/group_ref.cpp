////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "conversation_enum.hpp"
#include "pfs/assert.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include "pfs/i18n.hpp"

namespace chat {

using BACKEND = backend::sqlite3::contact_manager;

static std::string const INSERT_MEMBER {
    "INSERT OR IGNORE INTO `{}` (`group_id`, `member_id`)"
    " VALUES (:group_id, :member_id)"
};

template <>
bool
contact_manager<BACKEND>::group_ref::add_member_unchecked (contact::id member_id)
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    try {
        auto stmt = rep.dbh->prepare(fmt::format(INSERT_MEMBER, rep.members_table_name));

        PFS__ASSERT(!!stmt, "");

        stmt.bind(":group_id" , _id);
        stmt.bind(":member_id", member_id);

        auto res = stmt.exec();

        // If stmt.rows_affected() > 0 then new member added;
        // If stmt.rows_affected() == 0 then new member not added (already added earlier);
        // The last situation is not en error.
        return stmt.rows_affected() > 0;
    } catch (debby::error ex) {
        error err {errc::storage_error
            , fmt::format("add member {} to group {} failure {}:"
                , member_id, _id, ex.what())};
        throw err;
    }

    return false;
}

template <>
bool
contact_manager<BACKEND>::group_ref::add_member (contact::id member_id)
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    if (member_id != rep.me.contact_id) {
        auto c = rep.contacts->get(member_id);

        if (c.contact_id == contact::id{}) {
            error err {errc::contact_not_found
                , to_string(member_id)
                , "contact not found"};
            throw err;
        }

        if (c.type != conversation_enum::person) {
            error err {errc::unsuitable_group_member
                , to_string(member_id)
                , "member must be a person to add to group"};
            throw err;
        }
    }

    return add_member_unchecked(member_id);
}

static std::string const REMOVE_MEMBER {
    "DELETE from `{}` WHERE `group_id` = :group_id AND `member_id` = :member_id"
};

template <>
bool
contact_manager<BACKEND>::group_ref::remove_member (contact::id member_id)
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    try {
        auto stmt = rep.dbh->prepare(fmt::format(REMOVE_MEMBER
            , rep.members_table_name));

        PFS__ASSERT(!!stmt, "");

        stmt.bind(":group_id", _id);
        stmt.bind(":member_id", member_id);

        auto res = stmt.exec();
        return stmt.rows_affected() > 0;
    } catch (debby::error ex) {
        throw error {errc::storage_error, tr::f_("remove member {} from group {}: {}"
            , member_id, _id, ex.what())};
    }

    return false;
}

static std::string const REMOVE_ALL_MEMBERS {
    "DELETE from `{}` WHERE `group_id` = :group_id"
};

template <>
void
contact_manager<BACKEND>::group_ref::remove_all_members ()
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    try {
        auto stmt = rep.dbh->prepare(fmt::format(REMOVE_ALL_MEMBERS
            , rep.members_table_name));

        PFS__ASSERT(!!stmt, "");

        stmt.bind(":group_id", _id);
        auto res = stmt.exec();
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }
}

static std::string const IS_MEMBER_OF {
    "SELECT COUNT(1) as count FROM `{}`"
    " WHERE `group_id` = :group_id AND `member_id` = :member_id"
};

template <>
bool
contact_manager<BACKEND>::group_ref::is_member_of (contact::id member_id) const
{
    PFS__ASSERT(_pmanager, "");

    std::size_t count = 0;
    auto & rep = _pmanager->_rep;

    try {
        auto stmt = rep.dbh->prepare(fmt::format(IS_MEMBER_OF, rep.members_table_name));

        PFS__ASSERT(!!stmt, "");

        stmt.bind(":group_id", _id);
        stmt.bind(":member_id", member_id);

        auto res = stmt.exec();

        if (res.has_more())
            count = res.get<std::size_t>(0);
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    return count > 0;
}

static std::string const SELECT_MEMBERS {
    "SELECT B.`id`, B.`creator_id`, B.`alias`, B.`avatar`, B.`description`, B.`type`"
    " FROM `{}` A JOIN `{}` B"
    " ON A.`group_id` = :group_id AND A.`member_id` = B.`id`"
};

template <>
std::vector<contact::contact>
contact_manager<BACKEND>::group_ref::members () const
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    std::vector<contact::contact> members;

    // Add own contact
    if (is_member_of(_pmanager->my_contact().contact_id)) {
        auto me = _pmanager->my_contact();
        contact::contact c;

        c.contact_id  = me.contact_id;
        c.creator_id  = me.contact_id;
        c.alias       = me.alias;
        c.avatar      = me.avatar;
        c.description = me.description;
        c.type        = conversation_enum::person;

        members.push_back(std::move(c));
    }

    try {
        auto stmt = rep.dbh->prepare(fmt::format(SELECT_MEMBERS
            , rep.members_table_name, rep.contacts_table_name));

        PFS__ASSERT(!!stmt, "");

        stmt.bind(":group_id", _id);

        auto res = stmt.exec();

        while (res.has_more()) {
            contact::contact c;

            res["id"]          >> c.contact_id;
            res["creator_id"]  >> c.creator_id;
            res["alias"]       >> c.alias;
            res["avatar"]      >> c.avatar;
            res["description"] >> c.description;
            res["type"]        >> c.type;

            members.push_back(std::move(c));
            res.next();
        }
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    return members;
}

static std::string const SELECT_MEMBER_IDS {
    "SELECT `member_id` FROM `{}` WHERE `group_id` = :group_id"
};

template <>
std::vector<contact::id>
contact_manager<BACKEND>::group_ref::member_ids () const
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    std::vector<contact::id> members;

    try {
        auto stmt = rep.dbh->prepare(fmt::format(SELECT_MEMBER_IDS
            , rep.members_table_name));

        PFS__ASSERT(!!stmt, "");

        stmt.bind(":group_id", _id);

        auto res = stmt.exec();

        while (res.has_more()) {
            contact::id member_id;
            res["member_id"] >> member_id;
            members.push_back(std::move(member_id));
            res.next();
        }
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    return members;
}

static std::string const MEMBER_COUNT {
    "SELECT COUNT(1) as count FROM `{}` WHERE `group_id` = :group_id"
};

template <>
std::size_t contact_manager<BACKEND>::group_ref::count () const
{
    PFS__ASSERT(_pmanager, "");

    std::size_t count = 0;
    auto & rep = _pmanager->_rep;

    try {
        auto stmt = rep.dbh->prepare(fmt::format(MEMBER_COUNT, rep.members_table_name));

        PFS__ASSERT(!!stmt, "");

        stmt.bind(":group_id", _id);

        auto res = stmt.exec();

        if (res.has_more())
            count = res.get<std::size_t>(0);
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    return count;
}

template <>
member_difference_result
contact_manager<BACKEND>::group_ref::update (std::vector<contact::id> members)
{
    member_difference_result r;

    auto current_members = member_ids();
    auto diffs = member_difference(std::move(current_members), std::move(members));

    for (auto const & member_id: diffs.removed) {
        if (remove_member(member_id))
            r.removed.push_back(member_id);
    }

    for (auto const & member_id: diffs.added) {
        if (add_member(member_id))
            r.added.push_back(member_id);
    }

    return r;
}

} // namespace chat
