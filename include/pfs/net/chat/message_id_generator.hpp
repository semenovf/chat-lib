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

class message_id_generator
{
public:
    using type = uuid_t;

public:
    message_id_generator () {}

    type next () noexcept
    {
        return generate_uuid);
    }
};

}}} // namespace pfs::net::chat
