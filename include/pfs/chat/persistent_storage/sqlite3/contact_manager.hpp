////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact_list.hpp"
#include "group_list.hpp"
#include "database_traits.hpp"
#include "pfs/chat/basic_contact_manager.hpp"
#include "pfs/chat/contact.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

struct contact_manager_traits
{
    using database_handle_type = database_handle_t;
    using contact_list_type = contact_list;
    using group_list_type = group_list;
};

class contact_manager final
    : public basic_contact_manager<contact_manager, contact_manager_traits>
{
    friend class basic_contact_manager<contact_manager, contact_manager_traits>;

    using base_class = basic_contact_manager<contact_manager, contact_manager_traits>;
    using failure_handler_type = base_class::failure_handler_type;

private:
    database_handle_t _dbh;
    std::string  _contacts_table_name;
    std::string  _members_table_name;
    std::string  _followers_table_name;
    contact_list _contacts;
    group_list   _groups;

protected:
    bool ready () const noexcept
    {
        return !!_dbh;
    }

    bool wipe_impl ();

    auto contacts_impl () noexcept -> contact_list &
    {
        return _contacts;
    }

    auto contacts_impl () const noexcept -> contact_list const &
    {
        return _contacts;
    }

    auto groups_impl () noexcept -> group_list &
    {
        return _groups;
    }

    auto groups_impl () const noexcept -> group_list const &
    {
        return _groups;
    }

private:
    contact_manager () = delete;
    contact_manager (contact_manager const & other) = delete;
    contact_manager (contact_manager && other) = delete;
    contact_manager & operator = (contact_manager const & other) = delete;
    contact_manager & operator = (contact_manager && other) = delete;

public:
    contact_manager (database_handle_t dbh, std::function<void(std::string const &)> f);
    ~contact_manager () = default;
};

}}} // namespace chat::persistent_storage::sqlite3
