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
    , std::function<void(contact::contact_id
        , message::message_id
        , pfs::utc_time_point)> message_dispatched
    , error *)
{
    out->push(data);
    message_dispatched(addressee, message_id, pfs::current_utc_time_point());
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

// template <>
// void
// delivery_manager<BACKEND>::dispatch (contact::contact_id addressee
//     , message::message_credentials const & msg, error * perr)
// {
//     protocol::original_message m;
//     m.message_id    = msg.id;
//     m.author_id     = msg.author_id;
//     m.creation_time = msg.creation_time;
//     m.content       = msg.contents.has_value() ? to_string(*msg.contents) : std::string{};
//
//     auto data = serialize<protocol::original_message>(m);
//
//     error err;
//
//     if (!_rep.send_message(addressee, data, [] (pfs::utc_time_point dispatched_time) {
//             /*dispatched notification callback*/
//         }, & err)) {
//
//         if (perr) *perr = err; else CHAT__THROW(err);
//     }
// }

} // namespace chat
