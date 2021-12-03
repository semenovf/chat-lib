////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "entity_storage.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/exports.hpp"
#include <functional>
#include <string>

namespace pfs {
namespace chat {
namespace contact {

PFS_CHAT__EXPORT class contact_list: public entity_storage<contact_list>
{
    friend class entity_storage<contact_list>;

    using database_handle = entity_storage<contact_list>::database_handle;

protected:
    bool open_impl ();
    void close_impl () {};

    // Wipe data from database
    void wipe_impl ();

public:
    contact_list (database_handle dbh, std::string const & table_name = "contacts");

    /**
     * Save contact into database.
     */
    bool save (contact const & c);

    /**
     * Load contact specified by @a id from database.
     */
    optional<contact> load (contact_id id);

    /**
     * Fetch all contacts from database and process them by @a f
     */
    void all_of (std::function<void(contact const &)> f);
};

}}} // namespace pfs::chat::contact
