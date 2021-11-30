////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "common.hpp"
#include <string>

namespace pfs {
namespace net {
namespace chat {
namespace sqlite3 {

template <>
inline std::string field_type<std::string> () { return "TEXT"; };

template <>
struct storage_type<std::string>
{
    using type = std::string;
};

template <>
inline storage_type<std::string>::type encode (std::string const & orig)
{
    return orig;
}

template <>
inline bool decode (std::string const & orig, std::string * target)
{
    *target = orig;
    return true;
}

}}}} // namespace pfs::net::chat::sqlite3


