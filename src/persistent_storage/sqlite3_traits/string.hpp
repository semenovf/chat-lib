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
namespace chat {
namespace sqlite3 {

template <>
struct field_type<std::string>
{
    static std::string s () { return "TEXT"; };
};

template <>
struct storage_type<std::string>
{
    using type = std::string;
};

}}} // namespace pfs::chat::sqlite3


