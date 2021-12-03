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
#include "pfs/uuid.hpp"

namespace pfs {
namespace chat {
namespace sqlite3 {

template <>
struct field_type<uuid_t>
{
    static std::string s () { return "TEXT"; };
};

template <>
struct storage_type<uuid_t>
{
    using type = std::string;
};

template <>
struct codec<uuid_t>
{
    static typename storage_type<uuid_t>::type encode (uuid_t const & orig)
    {
        return std::to_string(orig);
    }

    static bool decode (typename storage_type<uuid_t>::type const & orig, uuid_t * target)
    {
        optional<uuid_t> o = from_string<uuid_t>(orig);

        if (!o.has_value())
            return false;

        *target = *o;
        return true;
    }
};

}}} // namespace pfs::chat::sqlite3
