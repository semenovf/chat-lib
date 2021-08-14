////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include "peer.hpp"
#include "types.hpp"
#include <list>
#include <map>

namespace pfs {
namespace chat {

PFS_CHAT_DLL_API class in_memory_peer_storage final
{
    using peers_collection = std::list<peer>;
    using peer_iterator    = peers_collection::iterator;
    using peers_index      = std::map<uuid_t, peer_iterator>;

    peers_collection _peers;
    peers_index      _peers_index;

public:
    bool has_peer (uuid_t id) const;
    void add_peer (peer && p);
    void remove_peer (uuid_t id);
    peer * peer_by_id (uuid_t id) const;

    std::size_t peers_count () const noexcept
    {
        return _peers.size();
    }
};

}} // namespace pfs::chat

