////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.05.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include <type_traits>

CHAT__NAMESPACE_BEGIN

template <typename EnumClass>
inline constexpr int sort_flags (EnumClass by, EnumClass order)
{
    return static_cast<typename std::underlying_type<EnumClass>::type>(by)
        | static_cast<typename std::underlying_type<EnumClass>::type>(order);
}

template <typename EnumClass>
inline constexpr bool sort_flag_on (typename std::underlying_type<EnumClass>::type flags
    , EnumClass flag)
{
    return flags
        & static_cast<typename std::underlying_type<EnumClass>::type>(flag);
}

CHAT__NAMESPACE_END
