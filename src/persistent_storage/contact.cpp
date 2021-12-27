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
        default:
            break;
    }

    return pfs::nullopt;
}

}} // namespace chat::contact
