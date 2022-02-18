////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
//      2022.02.17 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/group_list.hpp"
#include "pfs/chat/backend/sqlite3/group_list.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"

namespace chat {

using namespace debby::sqlite3;

namespace backend {
namespace sqlite3 {

group_list::rep_type
group_list::make (shared_db_handle dbh
    , std::string const & contacts_table_name
    , std::string const & members_table_name
    , std::shared_ptr<contact_list_type> contacts
    , error *)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.contacts_table_name = contacts_table_name;
    rep.members_table_name  = members_table_name;
    rep.contacts = contacts;

    return rep;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::group_list

template <>
group_list<BACKEND>::group_list (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
std::size_t
group_list<BACKEND>::count () const
{
    return _rep.contacts->count (contact::type_enum::group);
}

template <>
int
group_list<BACKEND>::add (contact::group const & g, error * perr)
{
    return _rep.contacts->add(g, perr);
}

template <>
int
group_list<BACKEND>::update (contact::group const & g, error * perr)
{
    return _rep.contacts->update(g, perr);
}

template <>
contact::group
group_list<BACKEND>::get (contact::contact_id id, error * perr) const
{
    error err;
    auto c = _rep.contacts->get(id, & err);

    if (err) {
        if (perr) *perr = err; else CHAT__THROW(err);
        return contact::group{};
    }

    if (c.type != contact::type_enum::group) {
        auto err = error{errc::group_not_found
            , fmt::format("group not found by id: #{}", to_string(id))};
        if (perr) *perr = err; else CHAT__THROW(err);
        return contact::group{};
    }

    return contact::group{c.id, c.alias};
}

namespace {
std::string const INSERT_MEMBER {
    "INSERT OR IGNORE INTO `{}` (`group_id`, `member_id`)"
    " VALUES (:group_id, :member_id)"
};
} // namespace

template <>
bool
group_list<BACKEND>::add_member(contact::contact_id group_id
    , contact::contact_id member_id
    , error * perr)
{
    error err;
    auto g = get(group_id, & err);

    if (err) {
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    auto c = _rep.contacts->get(member_id, & err);

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
    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_MEMBER, _rep.members_table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id" , to_storage(g.id), false, & storage_err)
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
                , to_string(g.id))
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
std::string const SELECT_MEMBERS {
    "SELECT B.`id`, B.`alias`, B.`type` FROM `{}` A JOIN `{}` B"
    " ON A.`group_id` = :group_id AND A.`member_id` = B.`id`"
};
} // namespace

template <>
std::vector<contact::contact>
group_list<BACKEND>::members (contact::contact_id group_id, error * perr) const
{
    debby::error storage_err;
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_MEMBERS
        , _rep.members_table_name, _rep.contacts_table_name), true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id", to_storage(group_id), false, & storage_err);

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
            , fmt::format("fetch members failure for group: #{}", to_string(group_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return std::vector<contact::contact>{};
    }

    return members;
}

} // namespace chat
