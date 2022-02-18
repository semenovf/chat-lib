////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/contact.hpp"
#include "pfs/debby/sqlite3/affinity_traits.hpp"
#include "pfs/debby/sqlite3/cast_traits.hpp"

namespace debby {
namespace sqlite3 {

template <>
struct affinity_traits<chat::contact::type_enum> : integral_affinity_traits
{};

template <>
struct cast_traits<chat::contact::type_enum>
{
    using storage_type = typename affinity_traits<chat::contact::type_enum>::storage_type;

    static storage_type to_storage (chat::contact::type_enum const & value)
    {
        return static_cast<storage_type>(value);
    }

    static pfs::optional<chat::contact::type_enum> to_native (storage_type const & value)
    {
        return chat::contact::to_type_enum(value);
    }
};

}} // namespace debby::sqlite3
