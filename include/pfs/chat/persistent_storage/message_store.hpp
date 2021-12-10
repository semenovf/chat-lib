////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "entity_storage.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/exports.hpp"
#include <functional>
#include <string>
#include <vector>

namespace pfs {
namespace chat {
namespace message {

PFS_CHAT__EXPORT class message_store: public entity_storage<message_store>
{
    friend class entity_storage<message_store>;
    using database_handle = entity_storage<message_store>::database_handle;

private:
    route_enum _route {route_enum::incoming};

protected:
    bool open_helper (route_enum route);

    bool open_impl ()
    {
        return open_helper(_route);
    }

    void close_impl () {};

    // Wipe data from database
    void wipe_impl ();

public:
    message_store (database_handle dbh, route_enum route);
    message_store (database_handle dbh, route_enum route, std::string const & table_name);

    /**
     * Save message credentials into database.
     */
    bool save (credentials const & m);

    /**
     * Load message credentials specified by @a id from database.
     *
     * @details For incoming messages and individual outgoing messages loads
     *          unique message credentials.
     *          For outgoing group message loads message credentials for group
     *          message and appropriate individual messages.
     */
    std::vector<credentials> load (message_id id);

    /**
     * Fetch all contacts from database and process them by @a f
     */
    void all_of (std::function<void(credentials const &)> f);
};

}}} // namespace pfs::chat::message
