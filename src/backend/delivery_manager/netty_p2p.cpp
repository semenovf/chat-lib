////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/protocol.hpp"
#include "pfs/chat/serializer.hpp"
#include "pfs/chat/delivery_manager.hpp"
#include "pfs/chat/backend/delivery_manager/netty_p2p.hpp"

namespace chat {

namespace backend {
namespace delivery_manager {

netty_p2p::rep
netty_p2p::make ()
{
    // TODO Implement
    rep r;
    return r;
}

result_status netty_p2p::rep::send_message (contact::contact_id addressee
    , message::message_id message_id
    , std::string const & data
    , chat::delivery_manager<netty_p2p>::message_dispatched_callback message_dispatched
    , chat::delivery_manager<netty_p2p>::message_delivered_callback message_delivered
    , chat::delivery_manager<netty_p2p>::message_read_callback message_read)
{
    // TODO Implement
//     message_dispatched(addressee, message_id, pfs::current_utc_time_point());
//     message_delivered(addressee, message_id, pfs::current_utc_time_point());
//     message_read(addressee, message_id, pfs::current_utc_time_point());
    return result_status{};
}

}} // namespace backend::delivery_manager

#define BACKEND backend::delivery_manager::netty_p2p

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

