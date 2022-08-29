////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.08.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include <vector>

namespace chat {

struct member_difference_result
{
    std::vector<contact::id> added;
    std::vector<contact::id> removed;
};

member_difference_result member_difference (
      std::vector<contact::id> old_members
    , std::vector<contact::id> new_members);

} // namespace chat
