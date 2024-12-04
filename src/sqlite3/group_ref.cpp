////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.10 Initial version.
//      2024.11.28 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "contact_list_impl.hpp"
#include "contact_manager_impl.hpp"
#include "chat/contact_manager.hpp"
#include <pfs/i18n.hpp>

CHAT__NAMESPACE_BEGIN

using contact_manager_t = contact_manager<storage::sqlite3>;

namespace storage {

static bool is_member_of (sqlite3::contact_manager const & rep, contact::id group_id, contact::id member_id)
{
    static char const * IS_MEMBER_OF = "SELECT COUNT(1) as count FROM \"{}\""
        " WHERE group_id = :group_id AND member_id = :member_id";

    std::size_t count = 0;

    debby::error err;
    auto stmt = rep.pdb->prepare_cached(fmt::format(IS_MEMBER_OF, rep.members_table_name), & err);

    if (!err) {
        stmt.bind(":group_id", group_id, & err)
            && stmt.bind(":member_id", member_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more())
                    count = res.get_or<std::size_t>(0, 0);
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return count > 0;
}

std::size_t count (sqlite3::contact_manager const & rep, contact::id group_id)
{
    static char const * MEMBER_COUNT =
        "SELECT COUNT(1) as count FROM \"{}\" WHERE group_id = :group_id";

    std::size_t count = 0;

    debby::error err;
    auto stmt = rep.pdb->prepare_cached(fmt::format(MEMBER_COUNT, rep.members_table_name), & err);

    if (!err) {
        stmt.bind(":group_id", group_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more())
                    count = res.get_or<std::size_t>(0, 0);
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return count;
}

std::vector<contact::id>
member_ids (sqlite3::contact_manager const & rep, contact::id group_id)
{
    static char const * SELECT_MEMBER_IDS = "SELECT member_id FROM \"{}\" WHERE group_id = :group_id";

    std::vector<contact::id> members;

    debby::error err;

    auto stmt = rep.pdb->prepare_cached(fmt::format(SELECT_MEMBER_IDS, rep.members_table_name), & err);

    if (!err) {
        stmt.bind(":group_id", group_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                while (res.has_more()) {
                    auto member_id = res.get<contact::id>("member_id", & err);

                    if (err)
                        break;

                    members.push_back(std::move(*member_id));
                    res.next();
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return members;
}

std::vector<contact::contact>
members (sqlite3::contact_manager const & rep, contact::id group_id, contact::person const & me)
{
    static char const * SELECT_MEMBERS = "SELECT B.id, B.creator_id"
        ", B.alias, B.avatar, B.description, B.extra, B.type"
        " FROM \"{}\" A JOIN \"{}\" B"
        " ON A.group_id = :group_id AND A.member_id = B.id";

    std::vector<contact::contact> members;

    // Add own contact
    if (is_member_of(rep, group_id, me.contact_id)) {
        contact::contact c;

        c.contact_id  = me.contact_id;
        c.creator_id  = me.contact_id;
        c.alias       = me.alias;
        c.avatar      = me.avatar;
        c.description = me.description;
        c.extra       = me.extra;
        c.type        = chat_enum::person;

        members.push_back(std::move(c));
    }

    debby::error err;

    auto stmt = rep.pdb->prepare_cached(fmt::format(SELECT_MEMBERS, rep.members_table_name
        , rep.contacts_table_name), & err);

    if (!err) {
        stmt.bind(":group_id", group_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                while (res.has_more()) {
                    contact::contact c;
                    sqlite3::contact_list::fill_contact(res, c);
                    c.contact_id = res.get_or("id", contact::id{});
                    members.push_back(std::move(c));
                    res.next();
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return members;
}

} // namespace storage

template <>
bool contact_manager_t::group_ref::add_member_unchecked (contact::id member_id)
{
    static char const * INSERT_MEMBER = "INSERT OR IGNORE INTO \"{}\""
        " (group_id, member_id) VALUES (:group_id, :member_id)";

    auto & rep = *_pmanager->_d;
    debby::error err;

    auto stmt = rep.pdb->prepare_cached(fmt::format(INSERT_MEMBER, rep.members_table_name), & err);

    if (!err) {
        stmt.bind(":group_id" , _id, & err)
            && stmt.bind(":member_id", member_id, & err);

            if (!err) {
                auto res = stmt.exec(& err);

                if (!err) {
                    // If stmt.rows_affected() > 0 then new member added;
                    // If stmt.rows_affected() == 0 then new member not added (already added earlier);
                    // The last situation is not en error.
                    return res.rows_affected() > 0;
                }
            }
    }

    if (err) {
        throw error {
              errc::storage_error
            , tr::f_("add member {} to group {} failure", member_id, _id)
            , err.what()
        };
    }

    return false;
}

template <>
bool contact_manager_t::group_ref::add_member (contact::id member_id)
{
    auto & rep = *_pmanager->_d;

    if (member_id != rep.my_contact_id) {
        auto c = _pmanager->get(member_id);

        if (c.contact_id == contact::id{})
            throw error { errc::contact_not_found, to_string(member_id)};


        if (c.type != chat_enum::person) {
            throw error {
                  errc::unsuitable_group_member, to_string(member_id)
                , tr::_("member must be a person to add to group")
            };
        }
    }

    return add_member_unchecked(member_id);
}

template <>
bool contact_manager_t::group_ref::remove_member (contact::id member_id)
{
    static char const * REMOVE_MEMBER = "DELETE FROM \"{}\" WHERE"
        " group_id = :group_id AND member_id = :member_id";

    auto & rep = *_pmanager->_d;
    debby::error err;

    auto stmt = rep.pdb->prepare_cached(fmt::format(REMOVE_MEMBER, rep.members_table_name), & err);

    if (!err) {
        stmt.bind(":group_id", _id, & err)
            && stmt.bind(":member_id", member_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                return res.rows_affected() > 0;
            }
        }
    }

    if (err) {
        throw error {
              errc::storage_error
            , tr::f_("remove member {} from group {} failure", member_id, _id)
            , err.what()
        };
    }

    return false;
}

template <>
void contact_manager_t::group_ref::remove_all_members ()
{
    static char const * REMOVE_ALL_MEMBERS = "DELETE FROM \"{}\" WHERE group_id = :group_id";

    auto & rep = *_pmanager->_d;
    debby::error err;

    auto stmt = rep.pdb->prepare_cached(fmt::format(REMOVE_ALL_MEMBERS, rep.members_table_name), & err);

    if (!err) {
        stmt.bind(":group_id", _id, & err);

        if (!err) {
            stmt.exec(& err);
        }
    }

    if (err) {
        throw error {
              errc::storage_error
            , tr::f_("remove member all members from group {} failure", _id)
            , err.what()
        };
    }
}

template <>
bool contact_manager_t::group_const_ref::is_member_of (contact::id member_id) const
{
    return storage::is_member_of(*_pmanager->_d, _id, member_id);
}

template <>
bool contact_manager_t::group_ref::is_member_of (contact::id member_id) const
{
    return storage::is_member_of(*_pmanager->_d, _id, member_id);
}

template <>
std::vector<contact::contact> contact_manager_t::group_const_ref::members () const
{
    return storage::members(*_pmanager->_d, _id, _pmanager->my_contact());
}

template <>
std::vector<contact::contact> contact_manager_t::group_ref::members () const
{
    return storage::members(*_pmanager->_d, _id, _pmanager->my_contact());
}

template <>
std::vector<contact::id> contact_manager_t::group_const_ref::member_ids () const
{
    return storage::member_ids(*_pmanager->_d, _id);
}

template <>
std::vector<contact::id> contact_manager_t::group_ref::member_ids () const
{
    return storage::member_ids(*_pmanager->_d, _id);
}

template <>
std::size_t contact_manager_t::group_const_ref::count () const
{
    return storage::count(*_pmanager->_d, _id);
}

template <>
std::size_t contact_manager_t::group_ref::count () const
{
    return storage::count(*_pmanager->_d, _id);
}

template <>
member_difference_result contact_manager_t::group_ref::update (std::vector<contact::id> members)
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

CHAT__NAMESPACE_END
