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

namespace {
std::string const GROUP_NOT_FOUND    { "group not found by id: #{}" };
std::string const CONTACT_NOT_FOUND  { "contact not found by id: #{}" };
std::string const UNSUITABLE_MEMBER_ERROR { "member must be a person to add to group" };
std::string const ADD_MEMBER_ERROR   { "add member failure: #{} to #{}: {}" };
std::string const FETCH_MEMBER_ERROR { "fetch members failure for group: #{}: {}" };
} // namespace

group_list::group_list (database_handle_t dbh
    , contact_list & contacts
    , std::string const & contacts_table_name
    , std::string const & members_table_name
    , failure_handler_t & f)
    : _dbh(dbh)
    , _contacts(contacts)
    , _contacts_table_name(contacts_table_name)
    , _members_table_name(members_table_name)
    , _on_failure(f)
{}

pfs::optional<contact::group> group_list::get_impl (contact::contact_id id)
{
    auto opt = _contacts.get(id);

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
    , contact::contact_id member_id)
{
    auto g = get_impl(group_id);

    if (!g) {
        _on_failure(fmt::format(GROUP_NOT_FOUND, to_string(group_id)));
        return false;
    }

    auto c = _contacts.get(member_id);

    if (!c) {
        _on_failure(fmt::format(CONTACT_NOT_FOUND, to_string(group_id)));
        return false;
    }

    if (c->type != contact::type_enum::person) {
        _on_failure(UNSUITABLE_MEMBER_ERROR);
        return false;
    }

    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(INSERT_MEMBER, _members_table_name), true, & err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id" , to_storage(g->id), false, & err)
        && stmt.bind(":member_id", to_storage(c->id), false, & err);

    if (success) {
        auto res = stmt.exec(& err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        _on_failure(fmt::format(ADD_MEMBER_ERROR
            , to_string(c->id)
            , to_string(g->id)
            , err.what()));
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

std::vector<contact::contact> group_list::members_impl (contact::contact_id group_id) const
{
    debby::error err;
    auto stmt = _dbh->prepare(fmt::format(SELECT_MEMBERS
        , _members_table_name, _contacts_table_name), true, & err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":group_id", to_storage(group_id), false, & err);

    std::vector<contact::contact> members;

    if (success) {
        auto res = stmt.exec(& err);

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
        _on_failure(fmt::format(FETCH_MEMBER_ERROR, to_string(group_id), err.what()));
        return std::vector<contact::contact>{};
    }

    return members;
}

}}} // namespace chat::persistent_storage::sqlite3
