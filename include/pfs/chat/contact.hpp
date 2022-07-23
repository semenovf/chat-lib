////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/exports.hpp"
#include "pfs/time_point.hpp"
#include "pfs/universal_id.hpp"
#include <string>

namespace chat {
namespace contact {

using id = pfs::universal_id;

class id_generator
{
public:
    id_generator () {}

    id next () noexcept
    {
        return pfs::generate_uuid();
    }
};

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

struct contact
{
    id          contact_id;
    std::string alias;
    std::string avatar;     // Application specific image path/name/code.
    std::string description;
    id          creator_id; // For person same as ID.
    type_enum type;
};

struct person
{
    id          contact_id;
    std::string alias;
    std::string avatar;
    std::string description;
};

struct group
{
    id          contact_id;
    std::string alias;
    std::string avatar;
    std::string description;
    id          creator_id;
};

struct channel
{
    id          contact_id;
    std::string alias;
    std::string avatar;
    std::string description;
    id          creator_id;
};

template <typename T>
inline bool is_valid (T const & t)
{
    return t.contact_id != id{};
}

CHAT__EXPORT type_enum to_type_enum (int n);
CHAT__EXPORT std::string to_string (type_enum type);

}} // namespace chat::contact
