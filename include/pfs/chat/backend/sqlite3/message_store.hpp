////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2021.11.21 Refactored.
//      2022.02.17 Replaced with backend declaration.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "conversation.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/conversation.hpp"
#include "pfs/chat/error.hpp"

namespace chat {
namespace backend {
namespace sqlite3 {

struct message_store
{
    using conversation_type = chat::conversation<conversation>;

    struct rep_type
    {
        shared_db_handle dbh;
        contact::contact_id me;
    };

    static rep_type make (contact::contact_id me
        , shared_db_handle dbh
        , error * perr = nullptr);
};

}}} // namespace chat::backend::sqlite3
