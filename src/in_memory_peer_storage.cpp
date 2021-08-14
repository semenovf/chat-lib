////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/in_memory_peer_storage.hpp"

namespace pfs {
namespace chat {

PFS_CHAT_DLL_API bool in_memory_peer_storage::has_peer (uuid_t id) const
{
    return _peers_index.find(id) != _peers_index.end();
}

PFS_CHAT_DLL_API void in_memory_peer_storage::add_peer (peer && p)
{
    if (!has_peer(p.id)) {
        auto id = p.id;
        _peers.push_back(std::forward<peer>(p));
        _peers_index.insert(std::make_pair(id, std::prev(_peers.end())));
    }
}

PFS_CHAT_DLL_API void in_memory_peer_storage::remove_peer (uuid_t id)
{
    auto it = _peers_index.find(id);

    if (it != _peers_index.end()) {
        _peers.erase(it->second);
        _peers_index.erase(it);
    }
}

PFS_CHAT_DLL_API peer * in_memory_peer_storage::peer_by_id (uuid_t id) const
{
    auto it = _peers_index.find(id);
    auto result = (it != _peers_index.end())
        ? & (*it->second)
        : nullptr;
    return result;
}

}} // namespace pfs::chat
