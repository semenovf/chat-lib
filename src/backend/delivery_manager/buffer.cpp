////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.18 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/protocol.hpp"
#include "pfs/chat/serializer.hpp"
#include "pfs/chat/delivery_manager.hpp"
#include "pfs/chat/backend/delivery_manager/buffer.hpp"

namespace chat {

namespace backend {
namespace delivery_manager {

buffer::rep
buffer::make (std::shared_ptr<queue_type> out
    , std::shared_ptr<queue_type> in
    , error *)
{
    rep r;
    r.out = out;
    r.in = in;
    return r;
}

bool buffer::rep::send_message (contact::contact_id addressee
    , message::message_id message_id
    , std::string const & data
    , chat::delivery_manager<buffer>::message_dispatched_callback message_dispatched
    , chat::delivery_manager<buffer>::message_delivered_callback message_delivered
    , chat::delivery_manager<buffer>::message_read_callback message_read
    , error *)
{
    out->push(data);
    message_dispatched(addressee, message_id, pfs::current_utc_time_point());
    message_delivered(addressee, message_id, pfs::current_utc_time_point());
    message_read(addressee, message_id, pfs::current_utc_time_point());
    return true;
}

}} // namespace backend::delivery_manager

#define BACKEND backend::delivery_manager::buffer

template <>
delivery_manager<BACKEND>::delivery_manager (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
delivery_manager<BACKEND>::operator bool () const noexcept
{
    return true;
}

} // namespace chat