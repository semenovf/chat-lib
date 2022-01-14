
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2021.11.21 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "conversation.hpp"
#include "database_traits.hpp"
#include "pfs/chat/basic_message_store.hpp"
#include "pfs/chat/exports.hpp"
#include <map>
#include <string>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

struct message_store_traits
{
    using database_handle_type = database_handle_t;
    using conversation_type    = conversation;
};

class message_store final
    : public basic_message_store<message_store, message_store_traits>
{
    friend class basic_message_store<message_store, message_store_traits>;

    using base_class = basic_message_store<message_store, message_store_traits>;
    using failure_handler_type = typename base_class::failure_handler_type;

private:
    database_handle_t _dbh;
    std::map<contact::contact_id, conversation> _conversation_cache;

protected:
    auto ready () const noexcept -> bool
    {
        return !!_dbh;
    }

    auto begin_conversation_impl (contact::contact_id c) -> conversation_type &;

    auto wipe_impl () -> bool
    {
        return conversation::wipe_all(_dbh, this->on_failure);
    }

private:
    message_store () = delete;
    message_store (message_store const & other) = delete;
    message_store (message_store && other) = delete;
    message_store & operator = (message_store const & other) = delete;
    message_store & operator = (message_store && other) = delete;

public:
    message_store (database_handle_t dbh, failure_handler_type f);
    ~message_store () = default;
};

}}} // namespace chat::persistent_storage::sqlite3
