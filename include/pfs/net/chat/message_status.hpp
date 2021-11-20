////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/uuid.hpp"

namespace pfs {
namespace net {
namespace chat {

enum class message_status
{
      draft
    , dispatched
    , delivered
    , read
};

}}} // namespace pfs::net::chat


