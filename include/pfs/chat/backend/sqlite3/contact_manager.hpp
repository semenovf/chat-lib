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
#include "pfs/chat/exports.hpp"
#include <memory>
#include <string>

namespace chat {
namespace backend {
namespace sqlite3 {

struct contact_manager
{
    struct rep_type
    {
        shared_db_handle dbh;
        // contact::person  me;
        chat::contact::id my_contact_id;
        std::string my_contact_table_name;
        std::string contacts_table_name;
        std::string members_table_name;
        std::string followers_table_name;
    };

    static CHAT__EXPORT rep_type make (contact::person const & me
        , shared_db_handle dbh);
};

}}} // namespace chat::backend::sqlite3
