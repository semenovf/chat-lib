////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/chat_enum.hpp"
#include "pfs/chat/error.hpp"

CHAT__NAMESPACE_BEGIN

chat_enum to_chat_enum (int n)
{
    switch (static_cast<chat_enum>(n)) {
        case chat_enum::person:
        case chat_enum::group:
        case chat_enum::channel:
            return static_cast<chat_enum>(n);
        default:
            break;
    }

    throw error {make_error_code(errc::invalid_argument)};
    return chat_enum::person;
}

std::string to_string (chat_enum type)
{
    switch (type) {
        case chat_enum::person:
            return std::string{"person"};
        case chat_enum::group:
            return std::string{"group"};
        case chat_enum::channel:
            return std::string{"channel"};
        default:
            break;
    }

    return std::string{};
}

CHAT__NAMESPACE_END
