////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.11.28 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "chat/sqlite3.hpp"

CHAT__NAMESPACE_BEGIN

namespace storage {

class sqlite3::contact_manager
{
public:
    relational_database_t * pdb {nullptr};
    contact::id my_contact_id;
    std::string my_contact_table_name {"chat_me"};
    std::string contacts_table_name   {"chat_contacts"};
    std::string members_table_name    {"chat_members"};
    std::string followers_table_name  {"chat_channels"};

public:
    contact_manager (contact::person const & my_contact, relational_database_t & db);
};

} // namespace storage

CHAT__NAMESPACE_END
