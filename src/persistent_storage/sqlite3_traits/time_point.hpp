////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "common.hpp"
#include "pfs/time_point.hpp"

namespace pfs {
namespace chat {
namespace sqlite3 {

template <>
struct field_type<utc_time_point>
{
    static std::string s () { return "INTEGER"; };
};

template <>
struct storage_type<utc_time_point>
{
    using type = std::int64_t;
};

template <>
struct codec<utc_time_point>
{
    static typename storage_type<utc_time_point>::type encode (utc_time_point const & orig)
    {
        return to_millis(orig).count();
    }

    static bool decode (typename storage_type<utc_time_point>::type const & orig, utc_time_point * target)
    {
        target->value = from_millis(std::chrono::milliseconds{orig});
        return true;
    }
};

}}} // namespace pfs::chat::sqlite3


