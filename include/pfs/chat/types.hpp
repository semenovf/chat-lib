////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/optional.hpp"
#include <chrono>
#include <string>

namespace pfs {
namespace chat {

#if PFS_HAVE_STD_OPTIONAL
    template <typename T>
    using optional = std::optional<T>;
#else
    template <typename T>
    using optional = pfs::optional<T>;
#endif

// FIXME This temporary typedefs, need real implementations for these entities
using uuid_t = std::string;
using uri_t  = std::string;

using icon_id_t = std::uint32_t;

using timestamp_t = std::chrono::milliseconds;

}} // namespace pfs::chat

