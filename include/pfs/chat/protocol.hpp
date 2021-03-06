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

enum class packet_type_enum: std::int8_t {
      unknown_packet        = 0
    , contact_credentials   = 1
    , original_message      = 2
    , delivery_notification = 3
    , read_notification     = 4
    , edited_message        = 5
};

struct contact_credentials
{
    contact::contact contact;
};

struct original_message
{
    message::id message_id;
    contact::id author_id;
    pfs::utc_time_point creation_time;
    std::string         content;
};

struct delivery_notification
{
    message::id message_id;
    contact::id addressee_id;
    pfs::utc_time_point delivered_time;
};

struct read_notification
{
    message::id message_id;
    contact::id addressee_id;
    pfs::utc_time_point read_time;
};

struct edited_message
{
    message::id message_id;
    contact::id author_id;
    pfs::utc_time_point modification_time;
    std::string         content;
};

}} // namespace chat::protocol
