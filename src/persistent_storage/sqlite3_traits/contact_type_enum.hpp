////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/contact.hpp"
#include "pfs/debby/sqlite3/affinity_traits.hpp"
#include "pfs/debby/sqlite3/cast_traits.hpp"

namespace pfs {
namespace debby {
namespace sqlite3 {

using namespace pfs::chat;

template <>
struct affinity_traits<contact::type_enum> : integral_affinity_traits
{};

template <>
struct cast_traits<contact::type_enum>
{
    using storage_type = typename affinity_traits<contact::type_enum>::storage_type;

    static storage_type to_storage (contact::type_enum const & value)
    {
        return static_cast<storage_type>(value);
    }

    static optional<contact::type_enum> to_native (storage_type const & value)
    {
        return contact::to_type_enum(value);
    }
};

}}} // namespace pfs::debby::sqlite3
