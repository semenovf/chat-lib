////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
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
#include "pfs/chat/contact.hpp"
#include "pfs/chat/contact_list.hpp"
#include "pfs/chat/exports.hpp"
#include <memory>
#include <string>

namespace chat {
namespace backend {
namespace sqlite3 {

struct contact_manager
{
    using contact_list_type = chat::contact_list<contact_list>;

    struct rep_type
    {
        shared_db_handle dbh;
        contact::person  me;
        std::string      contacts_table_name;
        std::string      members_table_name;
        std::string      followers_table_name;
        std::shared_ptr<contact_list_type> contacts;
    };

    static CHAT__EXPORT rep_type make (contact::person const & me
        , shared_db_handle dbh);
};

}}} // namespace chat::backend::sqlite3
