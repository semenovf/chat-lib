////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/persistent_storage/sqlite3/contact_list.hpp"
#include <memory>

class ContactList
{
public:
    using underlying_type = chat::persistent_storage::sqlite3::contact_list;
    using contact_type    = chat::contact::contact_credentials;

public:
    ContactList (underlying_type && d) : _d(std::move(d)) {}

    int count () const
    {
        return _d.count();
    }

    pfs::optional<chat::contact::contact_credentials> get (int offset)
    {
        return _d.get(offset);
    }

private:
    underlying_type _d;
};
