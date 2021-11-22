////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/optional.hpp"

namespace pfs {
namespace net {
namespace chat {

#if PFS_HAVE_STD_OPTIONAL
    template <typename T>
    using optional = std::optional<T>;
#else
    template <typename T>
    using optional = pfs::optional<T>;
#endif

}}} // namespace pfs::net::chat



