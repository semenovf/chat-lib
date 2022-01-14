////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/persistent_storage/sqlite3/contact_manager.hpp"
#include <memory>

struct ContactManagerBuilder
{
    using type = chat::persistent_storage::sqlite3::contact_manager;
    std::unique_ptr<type> operator () ();
};
