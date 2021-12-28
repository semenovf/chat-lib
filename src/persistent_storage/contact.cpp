////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/contact.hpp"

namespace chat {
namespace contact {

CHAT__EXPORT pfs::optional<type_enum> to_type_enum (int n)
{
    switch (n) {
        case static_cast<int>(type_enum::person):
            return type_enum::person;
        case static_cast<int>(type_enum::group):
            return type_enum::person;
        case static_cast<int>(type_enum::channel):
            return type_enum::channel;
        default:
            break;
    }

    return pfs::nullopt;
}

CHAT__EXPORT std::string to_string (type_enum type)
{
    switch (type) {
        case type_enum::person:
            return std::string{"person"};
        case type_enum::group:
            return std::string{"group"};
        case type_enum::channel:
            return std::string{"channel"};
        default:
            break;
    }

    return std::string{};
}

}} // namespace chat::contact
