////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "message.hpp"

namespace chat {
namespace protocol {

enum class packet_type {
      original_message      = 1
    , delivery_notification = 2
    , read_notification     = 3
    , edited_message        = 4
};

struct original_message
{
    message::message_id message_id;
    contact::contact_id author_id;
    pfs::utc_time_point creation_time;
    std::string         content;
};

struct delivery_notification
{
    message::message_id message_id;
    contact::contact_id addressee_id;
    pfs::utc_time_point delivered_time;
};

struct read_notification
{
    message::message_id message_id;
    contact::contact_id addressee_id;
    pfs::utc_time_point read_time;
};

struct edited_message
{
    message::message_id message_id;
    contact::contact_id author_id;
    pfs::utc_time_point modification_time;
    std::string         content;
};

}} // namespace chat::protocol
