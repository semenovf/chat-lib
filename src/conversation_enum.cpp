////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/conversation_enum.hpp"
#include "pfs/chat/error.hpp"

namespace chat {

CHAT__EXPORT conversation_enum to_conversation_enum (int n)
{
    switch (static_cast<conversation_enum>(n)) {
        case conversation_enum::person:
        case conversation_enum::group:
        case conversation_enum::channel:
            return static_cast<conversation_enum>(n);
        default:
            break;
    }

    throw chat::error {chat::make_error_code(chat::errc::invalid_argument)};;
    return conversation_enum::person;
}

CHAT__EXPORT std::string to_string (conversation_enum type)
{
    switch (type) {
        case conversation_enum::person:
            return std::string{"person"};
        case conversation_enum::group:
            return std::string{"group"};
        case conversation_enum::channel:
            return std::string{"channel"};
        default:
            break;
    }

    return std::string{};
}

} // namespace chat
