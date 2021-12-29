////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "entity_storage.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/exports.hpp"

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

CHAT__EXPORT class file_cache: public entity_storage<file_cache>
{
    friend class entity_storage<file_cache>;
    using database_handle = entity_storage<file_cache>::database_handle;

protected:
    bool open_impl (database_handle dbh, std::string const & table_name);
    void close_impl () {};

    // Wipe data from database
    void wipe_impl ();

public:
    file_cache ();

    /**
     * Save file credentials into database.
     */
    bool save (message::file_credentials const & f);

    /**
     * Load file credentials specified by @a id from database.
     */
    std::vector<message::file_credentials> load (message::message_id msg_id);

    /**
     * Fetch all file credentials from database and process them by @a f
     */
    void all_of (std::function<void(message::file_credentials const &)> f);
};

}}} // namespace chat::persistent_storage::sqlite3
