////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.03 Initial version.
//      2021.12.13 `message_store` splitted into `incoming_message_store` and `outgoing_message_store`.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "entity_storage.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/exports.hpp"
#include <functional>
#include <string>
#include <vector>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

CHAT__EXPORT class incoming_message_store: public entity_storage<incoming_message_store>
{
    friend class entity_storage<incoming_message_store>;
    using database_handle = entity_storage<incoming_message_store>::database_handle;

protected:
    bool open_impl (database_handle dbh, std::string const & table_name);
    void close_impl ();
    void wipe_impl ();

public:
    incoming_message_store ();

    /**
     * Save message credentials into database.
     */
    bool save (message::credentials const & m);

    /**
     * Load message credentials specified by @a id from database.
     *
     * @details For incoming messages loads unique message credentials.
     */
    std::vector<message::credentials> load (message::message_id id);

    /**
     * Fetch all contacts from database and process them by @a f
     */
    void all_of (std::function<void(message::credentials const &)> f);
};

}}} // namespace chat::persistent_storage::sqlite3
