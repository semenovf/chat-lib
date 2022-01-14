////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "contact_type_enum.hpp"
#include "pfs/chat/persistent_storage/sqlite3/group_list.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;

group_list::group_list (database_handle_t dbh
    , contact_list & contacts
    , std::string const & contacts_table_name
    , std::string const & members_table_name)
    : _dbh(dbh)
    , _contacts(contacts)
    , _contacts_table_name(contacts_table_name)
    , _members_table_name(members_table_name)
{}

pfs::optional<contact::group> group_list::get_impl (contact::contact_id id, error * perr) const
{
    auto opt = _contacts.get(id, perr);

    if (opt && opt->type == contact::type_enum::group)
        return contact::group{opt->id, opt->alias};

    return pfs::nullopt;
}

namespace {
std::string const INSERT_MEMBER {
    "INSERT OR IGNORE INTO `{}` (`group_id`, `member_id`)"
    " VALUES (:group_id, :member_id)"
};
} // namespace

bool group_list::add_member_impl(contact::contact_id group_id
    , contact::contact_id member_id
    , error * perr)
{
    error err;
    auto g = get_impl(group_id, & err);

    if (err) {
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    if (!g) {
        auto err = error{errc::group_not_found, to_string(group_id)};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    auto c = _contacts.get(member_id, & err);

    if (err) {
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    if (!c) {
        auto err = error{errc::contact_not_found, to_string(member_id)};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    if (c->type != contact::type_enum::person) {
        auto err = error{errc::unsuitable_member
            , to_string(member_id)
            , "member must be a person to add to group"};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(INSERT_MEMBER, _members_table_name), true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id" , to_storage(g->id), false, & storage_err)
        && stmt.bind(":member_id", to_storage(c->id), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , fmt::format("add member failure: #{} to #{}"
                , to_string(c->id)
                , to_string(g->id))
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

std::vector<contact::contact> group_list::members_impl (contact::contact_id group_id
    , error * perr) const
{
    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_MEMBERS
        , _members_table_name, _contacts_table_name), true, & storage_err);
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
        auto err = error{errc::storage_error
            , fmt::format("fetch members failure for group: #{}"
                , to_string(group_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return std::vector<contact::contact>{};
    }

    return members;
}

}}} // namespace chat::persistent_storage::sqlite3
