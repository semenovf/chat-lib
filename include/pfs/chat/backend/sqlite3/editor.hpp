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
#include "pfs/chat/message.hpp"

namespace chat {
namespace backend {
namespace sqlite3 {

struct editor
{
    struct rep_type
    {
        shared_db_handle    dbh;
        std::string         table_name;
        message::message_id message_id;
        message::content    content;

        // - `true` if editor opened for message modification;
        // - `false` if editor opened for just created message.
        bool modification {false};
    };

    static rep_type make (message::message_id message_id
        , shared_db_handle dbh
        , std::string const & table_name);

    static rep_type make (message::message_id message_id
        , message::content && content
        , shared_db_handle dbh
        , std::string const & table_name);
};

}}} // namespace chat::backend::sqlite3
