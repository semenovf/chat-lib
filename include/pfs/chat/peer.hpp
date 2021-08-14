////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "types.hpp"
#include <string>

namespace pfs {
namespace chat {

struct peer
{
    uuid_t id;
    std::string alias;
    optional<icon_id_t> icon_id;
    optional<uri_t>     icon_uri;
};

}} // namespace pfs::chat
