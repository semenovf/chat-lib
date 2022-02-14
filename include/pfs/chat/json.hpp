////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/jeyson/json.hpp"

#if CHAT__JANSSON_ENABLED
#include "pfs/jeyson/backend/jansson.hpp"
#endif

namespace chat {

#if CHAT__JANSSON_ENABLED
using json = jeyson::json<jeyson::jansson_backend>;
#endif

} // namespace chat
