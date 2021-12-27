////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "ContactList.hpp"
#include <memory>

struct ContactListBuilder
{
    using type = ContactList;
    std::shared_ptr<ContactList> operator () ();
};
