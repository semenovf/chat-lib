////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/timestamp.hpp"
#include "pfs/fmt.hpp"

namespace pfs {
namespace chat {

PFS_CHAT_DLL_API std::string to_string (timestamp_t const & t)
{
    auto millis = to_millis(t).count() % 1000;
    return fmt::format("{0:%F %H:%M:%S}.{1:03} {0:%z}", t, millis);
}

}} // namespace pfs::chat



