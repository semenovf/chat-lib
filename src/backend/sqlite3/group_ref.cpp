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
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"

namespace chat {

using namespace debby::sqlite3;

#define BACKEND backend::sqlite3::contact_manager

namespace {
std::string const INSERT_MEMBER {
    "INSERT OR IGNORE INTO `{}` (`group_id`, `member_id`)"
    " VALUES (:group_id, :member_id)"
};
} // namespace

template <>
bool
contact_manager<BACKEND>::group_ref::add_member(contact::contact_id member_id
    , error * perr)
{
    error err;
    auto & rep = _pmanager->_rep;

    if (err) {
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    auto c = rep.contacts->get(member_id, & err);

    if (err) {
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    if (c.type != contact::type_enum::person) {
        error err {errc::unsuitable_member
            , to_string(member_id)
            , "member must be a person to add to group"};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    debby::error storage_err;
    auto stmt = rep.dbh->prepare(fmt::format(INSERT_MEMBER, rep.members_table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id" , to_storage(_id), false, & storage_err)
        && stmt.bind(":member_id", to_storage(c.id), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        error err{errc::storage_error
            , fmt::format("add member failure: #{} to #{}"
                , to_string(c.id)
                , to_string(_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    // If stmt.rows_affected() > 0 then new member added;
    // If stmt.rows_affected() == 0 then new member not added (already added earlier);
    // The last situation is not en error.
    return true;
}

namespace {
std::string const REMOVE_MEMBER {
    "DELETE from `{}` WHERE `group_id` = :group_id AND `member_id` = :member_id"
};
} // namespace

template <>
bool
contact_manager<BACKEND>::group_ref::remove_member (contact::contact_id member_id
    , error * perr)
{
    debby::error storage_err;
    auto & rep = _pmanager->_rep;
    auto stmt = rep.dbh->prepare(fmt::format(REMOVE_MEMBER, rep.members_table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id", to_storage(_id), false, & storage_err)
        && stmt.bind(":member_id", to_storage(member_id), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        error err{errc::storage_error
            , fmt::format("remove member failure: #{} from #{}"
                , to_string(member_id)
                , to_string(_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    return success;
}

namespace {
std::string const SELECT_MEMBERS {
    "SELECT B.`id`, B.`alias`, B.`type` FROM `{}` A JOIN `{}` B"
    " ON A.`group_id` = :group_id AND A.`member_id` = B.`id`"
};
} // namespace

template <>
std::vector<contact::contact>
contact_manager<BACKEND>::group_ref::members (error * perr) const
{
    debby::error storage_err;
    auto & rep = _pmanager->_rep;
    auto stmt = rep.dbh->prepare(fmt::format(SELECT_MEMBERS
        , rep.members_table_name, rep.contacts_table_name), true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id", to_storage(_id), false, & storage_err);

    std::vector<contact::contact> members;

    if (success) {
        auto res = stmt.exec(& storage_err);

        while (res.has_more()) {
            contact::contact c;
            input_record in {res};

            success = in["id"] >> c.id
                && in["alias"] >> c.alias
                && in["type"]  >> c.type;

            if (success)
                members.push_back(std::move(c));

            res.next();
        }

        if (res.is_error())
            success = false;
    }

    if (!success) {
        error err {errc::storage_error
            , fmt::format("fetch members failure for group: #{}", to_string(_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return std::vector<contact::contact>{};
    }

    return members;
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
    std::size_t count = 0;
    debby::error err;
    auto & rep = _pmanager->_rep;
    auto stmt = rep.dbh->prepare(fmt::format(IS_MEMBER_OF, rep.members_table_name)
        , true, & err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id" , to_storage(_id), false, & err)
        && stmt.bind(":member_id" , to_storage(member_id), false, & err);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.has_more()) {
            auto opt = res.get<std::size_t>(0);
            assert(opt.has_value());
            count = *opt;
            res.next();
        }

        if (res.is_error())
            success = false;
    }

    return success ? count > 0 : false;
}

} // namespace chat
