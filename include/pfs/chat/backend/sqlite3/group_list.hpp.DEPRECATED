////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
//      2022.02.16 Replaced with backend declaration.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "contact_list.hpp"
#include "pfs/chat/contact_list.hpp"
#include "pfs/chat/error.hpp"
#include <memory>

namespace chat {
namespace backend {
namespace sqlite3 {

struct group_list
{
    using contact_list_type = chat::contact_list<contact_list>;

    struct rep_type
    {
        shared_db_handle dbh;
        std::string      contacts_table_name;
        std::string      members_table_name;
        std::shared_ptr<contact_list_type> contacts;
    };

    static rep_type make (shared_db_handle dbh
        , std::string const & contacts_table_name
        , std::string const & members_table_name
        , std::shared_ptr<contact_list_type> contacts
        , error * perr = nullptr);
};

}}} // namespace chat::backend::sqlite3
