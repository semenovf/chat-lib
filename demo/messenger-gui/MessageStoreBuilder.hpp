////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/persistent_storage/sqlite3/message_store.hpp"
#include <memory>

struct MessageStoreBuilder
{
    using type = chat::persistent_storage::sqlite3::message_store;
    std::unique_ptr<type> operator () ();
};
