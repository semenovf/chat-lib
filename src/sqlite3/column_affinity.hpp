////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.11.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "chat/chat_enum.hpp"
#include <pfs/debby/data_definition.hpp>
#include <pfs/mime_enum.hpp>

namespace debby {

using table_t = table<backend_enum::sqlite3>;

template <> template <> char const * table_t::column_type_affinity<chat::chat_enum>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<mime::mime_enum>::value = "INTEGER";

} // namespace debby
