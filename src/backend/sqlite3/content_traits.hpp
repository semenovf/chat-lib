////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/message.hpp"
#include "pfs/debby/backend/sqlite3/cast_traits.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

template <> struct affinity_traits<chat::message::content> : text_affinity_traits {};
template <> struct affinity_traits<chat::message::content &> : text_affinity_traits {};
template <> struct affinity_traits<chat::message::content const> : text_affinity_traits {};
template <> struct affinity_traits<chat::message::content const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, chat::message::content>::value, void>::type>
{
    using storage_type = typename affinity_traits<chat::message::content>::storage_type;

    static storage_type to_storage (chat::message::content const & value)
    {
        return to_string(value);
    }

    static pfs::optional<chat::message::content> to_native (storage_type const & value)
    {
        return chat::message::content(value);
    }
};

}}} // namespace debby::backend::sqlite3
