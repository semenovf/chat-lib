////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.04 Initial version.
//      2022.02.17 Replaced with backend declaration.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "pfs/chat/exports.hpp"
#include "pfs/chat/message.hpp"

namespace chat {
namespace backend {
namespace sqlite3 {

struct editor
{
    struct rep_type
    {
        shared_db_handle dbh;
        std::string      table_name;
        message::id      message_id;
        message::content content;
    };

    static CHAT__EXPORT rep_type make (message::id message_id
        , shared_db_handle dbh
        , std::string const & table_name);

    static CHAT__EXPORT rep_type make (message::id message_id
        , message::content && content
        , shared_db_handle dbh
        , std::string const & table_name);
};

}}} // namespace chat::backend::sqlite3
