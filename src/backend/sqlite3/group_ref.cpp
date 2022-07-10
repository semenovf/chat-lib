////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"

namespace chat {

using BACKEND = backend::sqlite3::contact_manager;

namespace {
std::string const INSERT_MEMBER {
    "INSERT OR IGNORE INTO `{}` (`group_id`, `member_id`)"
    " VALUES (:group_id, :member_id)"
};
} // namespace

template <>
bool
contact_manager<BACKEND>::group_ref::add_member_unchecked (contact::contact_id member_id)
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    TRY {
        auto stmt = rep.dbh->prepare(fmt::format(INSERT_MEMBER, rep.members_table_name));

        CHAT__ASSERT(!!stmt, "");

        stmt.bind(":group_id" , _id);
        stmt.bind(":member_id", member_id);

        auto res = stmt.exec();

        // If stmt.rows_affected() > 0 then new member added;
        // If stmt.rows_affected() == 0 then new member not added (already added earlier);
        // The last situation is not en error.
        return stmt.rows_affected() > 0;
    } CATCH (debby::error ex) {
        error err {errc::storage_error
            , fmt::format("add member {} to group {} failure {}:"
                , member_id, _id, ex.what())};
        CHAT__THROW(err);
    }

    return false;
}

template <>
bool
contact_manager<BACKEND>::group_ref::add_member (contact::contact_id member_id)
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    if (member_id != rep.me.id) {
        auto c = rep.contacts->get(member_id);

        if (c.id == contact::contact_id{}) {
            error err {errc::contact_not_found
                , to_string(member_id)
                , "contact not found"};
            CHAT__THROW(err);
            return false;
        }

        if (c.type != contact::type_enum::person) {
            error err {errc::unsuitable_group_member
                , to_string(member_id)
                , "member must be a person to add to group"};
            CHAT__THROW(err);
            return false;
        }
    }

    return add_member_unchecked(member_id);
}

namespace {
std::string const REMOVE_MEMBER {
    "DELETE from `{}` WHERE `group_id` = :group_id AND `member_id` = :member_id"
};
} // namespace

template <>
void
contact_manager<BACKEND>::group_ref::remove_member (contact::contact_id member_id)
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;
    auto stmt = rep.dbh->prepare(fmt::format(REMOVE_MEMBER, rep.members_table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":group_id", _id);
    stmt.bind(":member_id", member_id);

    auto res = stmt.exec();
}

namespace {
std::string const REMOVE_ALL_MEMBERS {
    "DELETE from `{}` WHERE `group_id` = :group_id"
};
} // namespace

template <>
void
contact_manager<BACKEND>::group_ref::remove_all_members ()
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;
    auto stmt = rep.dbh->prepare(fmt::format(REMOVE_ALL_MEMBERS, rep.members_table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":group_id", _id);
    auto res = stmt.exec();
}

namespace {
std::string const IS_MEMBER_OF {
    "SELECT COUNT(1) as count FROM `{}`"
    " WHERE `group_id` = :group_id AND `member_id` = :member_id"
};
} // namespace

template <>
bool
contact_manager<BACKEND>::group_ref::is_member_of (contact::contact_id member_id) const
{
    PFS__ASSERT(_pmanager, "");

    std::size_t count = 0;
    auto & rep = _pmanager->_rep;
    auto stmt = rep.dbh->prepare(fmt::format(IS_MEMBER_OF, rep.members_table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":group_id", _id);
    stmt.bind(":member_id", member_id);

    auto res = stmt.exec();

    if (res.has_more())
        count = res.get<std::size_t>(0);

    return count > 0;
}

namespace {
std::string const SELECT_MEMBERS {
    "SELECT B.`id`, B.`creator_id`, B.`alias`, B.`avatar`, B.`description`, B.`type`"
    " FROM `{}` A JOIN `{}` B"
    " ON A.`group_id` = :group_id AND A.`member_id` = B.`id`"
};
} // namespace

template <>
std::vector<contact::contact>
contact_manager<BACKEND>::group_ref::members () const
{
    PFS__ASSERT(_pmanager, "");

    auto & rep = _pmanager->_rep;

    auto stmt = rep.dbh->prepare(fmt::format(SELECT_MEMBERS
        , rep.members_table_name, rep.contacts_table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":group_id", _id);

    std::vector<contact::contact> members;

    // Add own contact if need
    if (is_member_of(_pmanager->my_contact().id)) {
        auto me = _pmanager->my_contact();
        contact::contact c;

        c.id          = me.id;
        c.creator_id  = me.id;
        c.alias       = me.alias;
        c.avatar      = me.avatar;
        c.description = me.description;
        c.type        = contact::type_enum::person;

        members.push_back(std::move(c));
    }

    auto res = stmt.exec();

    while (res.has_more()) {
        contact::contact c;

        res["id"]          >> c.id;
        res["creator_id"]  >> c.creator_id;
        res["alias"]       >> c.alias;
        res["avatar"]      >> c.avatar;
        res["description"] >> c.description;
        res["type"]        >> c.type;

        members.push_back(std::move(c));
        res.next();
    }

    return members;
}

namespace {
std::string const MEMBER_COUNT {
    "SELECT COUNT(1) as count FROM `{}` WHERE `group_id` = :group_id"
};
} // namespace

template <>
std::size_t contact_manager<BACKEND>::group_ref::count () const
{
    PFS__ASSERT(_pmanager, "");

    std::size_t count = 0;
    auto & rep = _pmanager->_rep;
    auto stmt = rep.dbh->prepare(fmt::format(MEMBER_COUNT, rep.members_table_name));

    CHAT__ASSERT(!!stmt, "");

    stmt.bind(":group_id", _id);

    auto res = stmt.exec();

    if (res.has_more())
        count = res.get<std::size_t>(0);

    return count;
}

} // namespace chat
