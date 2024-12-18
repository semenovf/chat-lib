////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.05.18 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"

CHAT__NAMESPACE_BEGIN

enum class editor_mode {
      create //< Open editor for message creation
    , modify //< Open editor for message modification
};

CHAT__NAMESPACE_END
