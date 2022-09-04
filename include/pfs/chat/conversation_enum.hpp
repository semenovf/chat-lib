////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021, 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.09.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/exports.hpp"
#include <string>

namespace chat {

// There are three exchange modes
//      1. One-to-one   (personal, individual)
//      2. Many-to-many (group)
//      3. One-to-many  (channel)
enum class conversation_enum
{
      person = 1
    , group
    , channel
};

CHAT__EXPORT conversation_enum to_conversation_enum (int n);
CHAT__EXPORT std::string to_string (conversation_enum type);

} // namespace chat
