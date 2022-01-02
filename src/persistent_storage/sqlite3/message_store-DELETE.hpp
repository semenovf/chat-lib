////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2021.12.13 `message_store` now is a helper class for specialized
//                  classes: `incoming_message_store` and  `outgoing_message_store`.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/message.hpp"
#include "pfs/chat/persistent_storage/sqlite3/entity_storage.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

class message_store
{
    using database_handle = entity_storage_traits::database_handle;

    database_handle _dbh;
    std::string const & _table_name;

public:
    message_store (database_handle dbh, std::string const & table_name)
        : _dbh(dbh)
        , _table_name(table_name)
    {}

    void close () {}

    // For incoming messages and individual outgoing messages loads
    // unique message credentials.
    // For outgoing group message loads message credentials for group
    // message and appropriate individual messages.
    std::vector<message::credentials> load (message::message_id id);

    void all_of (std::function<void(message::credentials const &)> f);
};

}}} // namespace chat::persistent_storage::sqlite3
