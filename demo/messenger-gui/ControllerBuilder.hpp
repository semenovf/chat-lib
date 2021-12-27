////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/messenger_controller.hpp"
#include <memory>

struct ControllerBuilder
{
    using type = chat::messenger_controller_st;

    std::unique_ptr<type> operator () ();
};
