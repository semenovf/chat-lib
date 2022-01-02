////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <map>
#include <vector>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

template <typename EntityKey, typename EntityType>
class in_memory_cache
{
    static constexpr std::size_t DEFAULT_CACHE_WINDOW_SIZE = 100;

    int offset;
    int limit;
    bool dirty;

    //std::vector<contact::contact> data;
    std::vector<EntityType> data;

    //std::map<contact::contact_id, std::size_t> map;
    std::map<EntityKey, std::size_t> map;
};

}}} // namespace chat::persistent_storage::sqlite3
