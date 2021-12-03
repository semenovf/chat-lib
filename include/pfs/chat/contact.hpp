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

namespace pfs {
namespace chat {
namespace contact {

using contact_id = uuid_t;

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
    utc_time_point last_activity;
};

PFS_CHAT__EXPORT optional<type_enum> to_type_enum (int n);

}}} // namespace pfs::chat::contact

namespace std {

inline string to_string (pfs::chat::contact::type_enum type)
{
    switch (type) {
        case pfs::chat::contact::type_enum::person:
            return string{"person"};
        case pfs::chat::contact::type_enum::group:
            return string{"group"};
        default:
            break;
    }

    return string{};
}

} // namespace std
