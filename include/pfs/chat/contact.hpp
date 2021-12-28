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

// There are three exchange modes
//      1. One-to-one   (individual)
//      2. Many-to-many (group)
//      3. One-to-many  (channel)
enum class type_enum
{
      person = 1
    , group
    , channel
};

struct contact_credentials
{
    contact_id  id;
    std::string alias;
    type_enum   type;
};

// struct group_credentials
// {
//     contact_id id;
//     std::vector<contact_id> members;
// };
//
// struct channel_credentials
// {
//     contact_id id;
//     std::vector<contact_id> followers;
// };

CHAT__EXPORT pfs::optional<type_enum> to_type_enum (int n);
CHAT__EXPORT std::string to_string (type_enum type);

}} // namespace chat::contact
