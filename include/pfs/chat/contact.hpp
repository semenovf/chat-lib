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
#include "pfs/uuid.hpp"
#include <string>

namespace chat {
namespace contact {

using contact_id = pfs::uuid_t;

class id_generator
{
public:
    using type = contact_id;

public:
    id_generator () {}

    type next () noexcept
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
    contact_id  id;
    contact_id  creator_id; // For person same as id
    std::string alias;
    std::string avatar; // application specific image path/name/code
    std::string description;
    type_enum type;
};

struct person
{
    contact_id  id;
    std::string alias;
    std::string avatar;
    std::string description;
};

struct group
{
    contact_id  id;
    contact_id  creator_id;
    std::string alias;
    std::string avatar;
    std::string description;
};

struct channel
{
    contact_id  id;
    contact_id  creator_id;
    std::string alias;
    std::string avatar;
    std::string description;
};

template <typename T>
inline bool is_valid (T const & t)
{
    return t.id != contact_id{};
}

CHAT__EXPORT type_enum to_type_enum (int n);
CHAT__EXPORT std::string to_string (type_enum type);

}} // namespace chat::contact
