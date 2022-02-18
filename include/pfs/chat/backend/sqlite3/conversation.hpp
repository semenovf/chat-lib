////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
//      2022.02.17 Replaced with backend declaration.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "db_traits.hpp"
#include "editor.hpp"
#include "pfs/chat/editor.hpp"
#include "pfs/chat/contact.hpp"

namespace chat {
namespace backend {
namespace sqlite3 {

struct conversation
{
    using editor_type = chat::editor<editor>;

    struct rep_type
    {
        shared_db_handle dbh;
        contact::contact_id me;
        contact::contact_id addressee;
        std::string table_name;
    };

    static rep_type make (contact::contact_id me
        , contact::contact_id addressee
        , shared_db_handle dbh
        , error * perr = nullptr);
};

}}} // namespace chat::backend::sqlite3
