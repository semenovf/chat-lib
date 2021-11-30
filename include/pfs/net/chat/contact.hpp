////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/time_point.hpp"
#include "pfs/uuid.hpp"
#include <string>

namespace pfs {
namespace net {
namespace chat {

using contact_id = uuid_t;

struct contact
{
    contact_id  id;
    std::string name;
    utc_time_point last_activity;
};

}}} // namespace pfs::net::chat

