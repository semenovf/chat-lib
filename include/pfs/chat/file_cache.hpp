////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.07.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/universal_id.hpp"

namespace chat {
namespace file {

using id = ::pfs::universal_id;

class id_generator
{
public:
    id_generator () {}

    id next () noexcept
    {
        return pfs::generate_uuid();
    }
};

template <typename Backend>
class cache
{
    using rep_type = typename Backend::rep_type;

public:
};

}} // namespace chat::file
