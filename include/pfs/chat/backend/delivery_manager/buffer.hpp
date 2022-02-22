////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.16 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/error.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/ring_buffer.hpp"
#include <functional>

namespace chat {
namespace backend {
namespace delivery_manager {

struct buffer
{
    using queue_type = pfs::ring_buffer<std::string, 256>;

    struct rep
    {
        std::shared_ptr<queue_type> out;
        std::shared_ptr<queue_type> in;

        bool send_message (contact::contact_id addressee
            , message::message_id message_id
            , std::string const & data
            , std::function<void(contact::contact_id
                , message::message_id
                , pfs::utc_time_point)> message_dispatched
            , error * perr = nullptr);
    };

    static rep make (std::shared_ptr<queue_type> out
        , std::shared_ptr<queue_type> in
        , error * perr = nullptr);
};

}}} // namespace chat::backend::delivery_manager
