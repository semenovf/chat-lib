////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2022.02.16 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/backend/sqlite3/db_traits.hpp"

namespace chat {
namespace backend {
namespace sqlite3 {

shared_db_handle make_handle (pfs::filesystem::path const & path, bool create_if_missing)
{
    auto presets = debby::backend::sqlite3::database::presets_enum::CONCURRENCY_PRESET;

    auto p = db_traits::database_type::make_unique(path, create_if_missing, presets);
    return std::move(p);
}

}}} // namespace chat::backend::sqlite3
