////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "ContactListBuilder.hpp"
#include "ControllerBuilder.hpp"
#include "pfs/chat/messenger.hpp"
#include <memory>

using Messenger = chat::messenger<
      ControllerBuilder
    , ContactListBuilder>;

using SharedMessenger = std::shared_ptr<Messenger>;
