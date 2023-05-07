////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.19 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/contact.hpp"
#include "pfs/chat/exports.hpp"
#include <map>
#include <vector>

namespace chat {
namespace backend {
namespace in_memory {

struct contact_list
{
    struct rep_type
    {
        std::vector<contact::contact> data;
        std::map<contact::id, std::size_t> map;
    };

    // Makes empty contact list
    static CHAT__EXPORT rep_type make ();
};

}}} // namespace chat::backend::in_memory

