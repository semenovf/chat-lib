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
#include "peer.hpp"

namespace pfs {
namespace chat {

template <typename PeerStorage>
class peer_manager final
{
    PeerStorage * _peer_storage_ptr {nullptr};

public:
    peer_manager (PeerStorage & peer_storage)
        : _peer_storage_ptr(& peer_storage)
    {}

    peer_manager () = delete;
    peer_manager (peer_manager const &) = delete;
    peer_manager & operator = (peer_manager const &) = delete;
    peer_manager (peer_manager &&) = default;
    peer_manager & operator = (peer_manager &&) = default;

    void peer_connected (peer && p)
    {
        _peer_storage_ptr->add_peer(std::forward<peer>(p));
    }

    void peer_disconnected (uuid_t peer_id)
    {
        _peer_storage_ptr->remove_peer(peer_id);
    }

    std::size_t peers_count () const noexcept
    {
        return _peer_storage_ptr->peers_count();
    }

    peer * peer_by_id (uuid_t id) const
    {
        return _peer_storage_ptr->peer_by_id(id);
    }
};

}} // namespace pfs::chat
