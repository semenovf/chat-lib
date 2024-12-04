////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include <map>
#include <set>
#include <string>

CHAT__NAMESPACE_BEGIN

class emoji_db final
{
    static std::set<std::string> _emojis;
    static std::map<std::string, std::string> _emoticons;

private:
    emoji_db () = delete;
    ~emoji_db () = delete;

public:
    static bool has (std::string const & emoji_id);
};

CHAT__NAMESPACE_END
