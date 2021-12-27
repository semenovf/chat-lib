////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/emitter.hpp"
#include "pfs/uuid.hpp"

namespace chat {

template <template <typename ...> class EmitterType>
class messenger_controller
{
    template <typename ...Args>
    using emitter_type = EmitterType<Args...>;

public: // signals
    emitter_type<std::string const &> failure;
};

using messenger_controller_st = messenger_controller<pfs::emitter>;
using messenger_controller_mt = messenger_controller<pfs::emitter_mt>;

} // namespace chat
