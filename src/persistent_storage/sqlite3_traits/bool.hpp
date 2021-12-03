////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "common.hpp"

namespace pfs {
namespace chat {
namespace sqlite3 {

template <>
struct field_type<bool>
{
    static std::string s () { return "INTEGER"; };
};

template <>
struct storage_type<bool>
{
    using type = int;
};

template <>
struct codec<bool>
{
    static typename storage_type<bool>::type encode (bool const & orig)
    {
        return orig ? 1 : 0;
    }

    static bool decode (typename storage_type<bool>::type const & orig, bool * target)
    {
        *target = *target == 0 ? false : true;
        return true;
    }
};

}}} // namespace pfs::chat::sqlite3



