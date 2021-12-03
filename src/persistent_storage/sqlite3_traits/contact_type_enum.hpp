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
#include "pfs/chat/contact.hpp"

namespace pfs {
namespace chat {
namespace sqlite3 {

template <>
struct field_type<contact::type_enum>
{
    static std::string s () { return "INTEGER"; };
};

template <>
struct storage_type<contact::type_enum>
{
    using type = int;
};

template <>
struct codec<contact::type_enum>
{
    static typename storage_type<contact::type_enum>::type encode (contact::type_enum const & orig)
    {
        return static_cast<storage_type<contact::type_enum>::type>(orig);
    }

    static bool decode (typename storage_type<contact::type_enum>::type const & orig, contact::type_enum * target)
    {
        auto res = contact::to_type_enum(orig);

        if (res.has_value())
            *target = *res;

        return res.has_value();
    }
};

}}} // namespace pfs::chat::sqlite3

