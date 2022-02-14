
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
#include "pfs/chat/error.hpp"
#include "pfs/chat/exports.hpp"
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

private:
    database_handle_t _dbh;

protected:
    auto ready () const noexcept -> bool
    {
        return !!_dbh;
    }

    bool wipe_impl (error * perr)
    {
        return conversation::wipe_all(_dbh, perr);
    }

    conversation_type conversation_impl (contact::contact_id my_id
        , contact::contact_id addressee_id) const;

private:
    message_store () = delete;
    message_store (message_store const & other) = delete;
    message_store (message_store && other) = delete;
    message_store & operator = (message_store const & other) = delete;
    message_store & operator = (message_store && other) = delete;

public:
    message_store (database_handle_t dbh);
    ~message_store () = default;
};

}}} // namespace chat::persistent_storage::sqlite3
