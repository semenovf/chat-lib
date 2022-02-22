////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "protocol.hpp"
#include "message.hpp"

namespace chat {

template <typename Packet>
std::string serialize (Packet const & msg);

template <typename Packet>
Packet deserialize (std::string const & data);

} // namespace chat
