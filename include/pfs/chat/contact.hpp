////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "conversation_enum.hpp"
#include "exports.hpp"
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

struct contact
{
    id          contact_id;
    std::string alias;
    std::string avatar;     // Application specific image path/name/code.
    std::string description;
    std::string extra;      // Extra data (implementation specific)
    id          creator_id; // For person same as ID.
    conversation_enum type;
};

struct person
{
    id          contact_id;
    std::string alias;
    std::string avatar;
    std::string description;
    std::string extra;
};

struct group
{
    id          contact_id;
    std::string alias;
    std::string avatar;
    std::string description;
    std::string extra;
    id          creator_id;
};

struct channel
{
    id          contact_id;
    std::string alias;
    std::string avatar;
    std::string description;
    std::string extra;
    id          creator_id;
};

template <typename T>
inline bool is_valid (T const & t) noexcept
{
    return t.contact_id != id{};
}

inline bool is_person (contact const & c) noexcept
{
    return c.type == conversation_enum::person;
}

inline bool is_group (contact const & c) noexcept
{
    return c.type == conversation_enum::group;
}

inline bool is_channel (contact const & c) noexcept
{
    return c.type == conversation_enum::channel;
}

}} // namespace chat::contact
