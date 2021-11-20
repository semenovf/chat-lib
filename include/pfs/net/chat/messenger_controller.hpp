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

namespace pfs {
namespace net {
namespace chat {

class messenger_controller
{
public: // signals
    pfs::emitter_mt<std::string const &> failure;
};

}}} // namespace pfs::net::chat



