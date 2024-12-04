////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "contact.hpp"
#include "file.hpp"
#include "message.hpp"

CHAT__NAMESPACE_BEGIN

namespace protocol {

enum class packet_enum: std::int8_t {
      unknown_packet        = 0
    , contact_credentials   = 1
    , group_members         = 2
    , regular_message       = 3
    , delivery_notification = 4
    , read_notification     = 5
    , file_request          = 6
    , file_error            = 7
};

struct contact_credentials
{
    contact::contact contact;
};

struct group_members
{
    contact::id group_id;
    std::vector<contact::id> members;
};

struct regular_message
{
    message::id message_id;
    contact::id author_id;
    contact::id chat_id;  // Target conversation ID (same as author for
                                  // personal conversation or group conversation ID)
    pfs::utc_time_point mod_time; // Creation or modification time
    std::string content;
};

struct delivery_notification
{
    message::id message_id;
    contact::id chat_id;
    pfs::utc_time_point delivered_time;
};

struct read_notification
{
    message::id message_id;
    contact::id chat_id;
    pfs::utc_time_point read_time;
};

struct file_request
{
    file::id file_id;
};

struct file_error
{
    file::id file_id;
};

} // namespace protocol

CHAT__NAMESPACE_END
