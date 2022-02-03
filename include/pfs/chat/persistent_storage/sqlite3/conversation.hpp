////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"
#include "editor.hpp"
#include "pfs/chat/basic_conversation.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/message.hpp"

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

class message_store;

struct conversation_store_traits
{
    using editor_type = editor;
};

class conversation final: public basic_conversation<conversation, conversation_store_traits>
{
    friend class basic_conversation<conversation, conversation_store_traits>;
    friend class message_store;

    using base_class = basic_conversation<conversation, conversation_store_traits>;

private:
    contact::contact_id _contact_id;
    std::string         _table_name;
    database_handle_t   _dbh;
    failure_handler_t   _on_failure;

protected:
    auto ready () const noexcept -> bool
    {
        return !!_dbh;
    }

    auto create_impl (contact::contact_id addressee_id) -> editor;
    auto count_impl () const -> std::size_t;
    auto wipe_impl () -> bool;
    auto unread_messages_count_impl () const -> std::size_t;

    // Wipes (drops) all conversations
    static auto wipe_all (database_handle_t dbh, failure_handler_t & on_failure) -> bool;

private:
    conversation () = delete;
    conversation (conversation const & other) = delete;
    conversation & operator = (conversation const & other) = delete;
    conversation & operator = (conversation && other) = delete;

    conversation (contact::contact_id c, database_handle_t dbh, failure_handler_t f);

public:
    conversation (conversation && other) = default;
    ~conversation () = default;
};

}}} // namespace chat::persistent_storage::sqlite3
