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
inline std::string field_type<uuid_t> () { return "TEXT"; };

template <>
struct storage_type<uuid_t>
{
    using type = std::string;
};

template <>
inline storage_type<uuid_t>::type encode (uuid_t const & orig)
{
    return std::to_string(orig);
}

template <>
inline bool decode (std::string const & orig, uuid_t * target)
{
    optional<uuid_t> o = from_string<uuid_t>(orig);

    if (!o.has_value())
        return false;

    *target = *o;
    return true;
}

}}} // namespace pfs::chat::sqlite3

