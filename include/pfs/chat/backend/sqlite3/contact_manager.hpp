////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2022.02.16 Replaced with backend declaration.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "contact_list.hpp"
#include "group_list.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/contact_list.hpp"
#include "pfs/chat/group_list.hpp"
#include <memory>
#include <string>

namespace chat {
namespace backend {
namespace sqlite3 {

struct contact_manager
{
    using contact_list_type = chat::contact_list<contact_list>;
    using group_list_type   = chat::group_list<group_list>;

    struct rep_type
    {
        shared_db_handle dbh;
        contact::person  me;
        std::string      contacts_table_name;
        std::string      members_table_name;
        std::string      followers_table_name;
        std::shared_ptr<contact_list_type> contacts;
        std::shared_ptr<group_list_type>   groups;
    };

    static rep_type make (contact::person const & me
        , shared_db_handle dbh
        , error * perr = nullptr);
};

}}} // namespace chat::backend::sqlite3
