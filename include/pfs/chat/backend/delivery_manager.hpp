////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.16 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/error.hpp"

namespace chat {
namespace backend {

struct delivery_manager
{
    struct rep_type {};

    static rep_type make (error * perr = nullptr);
};

}} // namespace chat::backend
