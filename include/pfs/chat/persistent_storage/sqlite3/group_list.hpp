////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"
#include "contact_list.hpp"
#include "pfs/chat/basic_group_list.hpp"

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

class contact_manager;

class group_list final: public basic_group_list<group_list>
{
    friend class basic_group_list<group_list>;
    friend class contact_manager;

    using base_class = basic_group_list<group_list>;

private:
    database_handle_t _dbh;
    contact_list      & _contacts;
    std::string const & _contacts_table_name;
    std::string const & _members_table_name;
    failure_handler_t & _on_failure;

protected:
    std::size_t count_impl () const
    {
        return _contacts.count (contact::type_enum::group);
    }

    int add_impl (contact::group const & g)
    {
        return _contacts.add(g);
    }

    int add_impl (contact::group && g)
    {
        return _contacts.add(std::move(g));
    }

    int update_impl (contact::group const & g)
    {
        return _contacts.update(g);
    }

    pfs::optional<contact::group> get_impl (contact::contact_id id);

    bool add_member_impl(contact::contact_id group_id
        , contact::contact_id member_id);

    std::vector<contact::contact> members_impl (contact::contact_id group_id) const;

private:
    group_list () = delete;
    ~group_list () = default;
    group_list (group_list const & other) = delete;
    group_list & operator = (group_list const & other) = delete;
    group_list (group_list && other) = delete;
    group_list & operator = (group_list && other) = delete;

    group_list (database_handle_t dbh
        , contact_list & contacts
        , std::string const & contacts_table_name
        , std::string const & members_table_name
        , failure_handler_t & f);
};

}}} // namespace chat::persistent_storage::sqlite3

