////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/exports.hpp"
#include "pfs/optional.hpp"
#include "pfs/time_point.hpp"
#include "pfs/uuid.hpp"
#include <string>

namespace chat {
namespace contact {

using contact_id = pfs::uuid_t;

enum class type_enum
{
      person = 1
    , group
};

struct contact
{
    contact_id  id;
    std::string name;
    std::string alias;
    type_enum   type;
    pfs::utc_time_point last_activity;
};

CHAT__EXPORT pfs::optional<type_enum> to_type_enum (int n);

inline std::string to_string (type_enum type)
{
    switch (type) {
        case type_enum::person:
            return std::string{"person"};
        case type_enum::group:
            return std::string{"group"};
        default:
            break;
    }

    return std::string{};
}

}} // namespace chat::contact
