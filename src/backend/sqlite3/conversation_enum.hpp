////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/conversation_enum.hpp"
#include "pfs/debby/backend/sqlite3/affinity_traits.hpp"
#include "pfs/debby/backend/sqlite3/cast_traits.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

template <>
struct affinity_traits<chat::conversation_enum> : integral_affinity_traits
{};

template <>
struct cast_traits<chat::conversation_enum>
{
    using storage_type = typename affinity_traits<chat::conversation_enum>::storage_type;

    static storage_type to_storage (chat::conversation_enum const & value)
    {
        return static_cast<storage_type>(value);
    }

    static chat::conversation_enum to_native (storage_type const & value)
    {
        return chat::to_conversation_enum(value);
    }
};

}}} // namespace debby::backend::sqlite3
