////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include <chrono>
#include <string>

namespace pfs {
namespace chat {

using timestamp_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

inline std::chrono::milliseconds to_millis (timestamp_t const & t)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch());
}

PFS_CHAT_DLL_API std::string to_string (timestamp_t const & t);

}} // namespace pfs::chat


