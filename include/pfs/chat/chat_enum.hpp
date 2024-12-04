////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.09.01 Initial version.
//      2024.11.29 Started V2.
//                 Renamed from conversation_enum` to `chat_enum`.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "pfs/chat/exports.hpp"
#include <string>

CHAT__NAMESPACE_BEGIN

// There are three exchange modes
//      1. One-to-one   (personal, individual)
//      2. Many-to-many (group)
//      3. One-to-many  (channel)
enum class chat_enum
{
      person = 1
    , group
    , channel
};

CHAT__EXPORT chat_enum to_chat_enum (int n);
CHAT__EXPORT std::string to_string (chat_enum type);

CHAT__NAMESPACE_END
