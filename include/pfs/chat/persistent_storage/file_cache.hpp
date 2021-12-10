////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "entity_storage.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/exports.hpp"
// #include <functional>
// #include <string>
// #include <vector>

namespace pfs {
namespace chat {
namespace message {

PFS_CHAT__EXPORT class file_cache: public entity_storage<file_cache>
{
    friend class entity_storage<file_cache>;
    using database_handle = entity_storage<file_cache>::database_handle;

private:
    message::route_enum _route {message::route_enum::incoming};

protected:
    bool open_helper (message::route_enum route);

    void close_impl () {};

    // Wipe data from database
    void wipe_impl ();

public:
    file_cache (database_handle dbh, route_enum route);
    file_cache (database_handle dbh, route_enum route, std::string const & table_name);

    bool open_impl ()
    {
        return open_helper(_route);
    }

    /**
     * Save file credentials into database.
     */
    bool save (file_credentials const & f);

    /**
     * Load file credentials specified by @a id from database.
     */
    std::vector<file_credentials> load (message_id msg_id);

    /**
     * Fetch all file credentials from database and process them by @a f
     */
    void all_of (std::function<void(file_credentials const &)> f);
};

}}} // namespace pfs::chat::message
