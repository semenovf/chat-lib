////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "types.hpp"

namespace pfs {
namespace chat {

//std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

struct message
{
    uuid_t id;
    uuid_t sender_id;
    uuid_t recipient_id;
    timestamp_t creation_time;
    timestamp_t received_time;
    timestamp_t read_time;
};

}} // namespace pfs::chat

