////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.08.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/member_difference.hpp"
#include <algorithm>

namespace chat {

member_difference_result
member_difference (std::vector<contact::id> old_members, std::vector<contact::id> new_members)
{
    std::sort(old_members.begin(), old_members.end());
    std::sort(new_members.begin(), new_members.end());

    member_difference_result r;

    std::set_difference(old_members.begin(), old_members.end()
        , new_members.begin(), new_members.end()
        , std::back_inserter(r.removed));

    std::set_difference(new_members.begin(), new_members.end()
        , old_members.begin(), old_members.end()
        , std::back_inserter(r.added));

    return r;
}

} // namespace chat
