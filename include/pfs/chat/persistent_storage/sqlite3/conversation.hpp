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
    contact::contact_id _my_id;
    contact::contact_id _addressee_id;
    std::string         _table_name;
    database_handle_t   _dbh;

protected:
    auto ready () const noexcept -> bool
    {
        return !!_dbh;
    }

    editor create_impl (error * perr);
    editor open_impl (message::message_id message_id, error * perr);
    std::size_t count_impl () const;
    bool wipe_impl (error * perr);
    void mark_dispatched_impl (message::message_id message_id);
    std::size_t unread_messages_count_impl () const;

    // Wipes (drops) all conversations
    static bool wipe_all (database_handle_t dbh, error * perr);

private:
    conversation (conversation const & other) = delete;
    conversation & operator = (conversation const & other) = delete;
    conversation & operator = (conversation && other) = delete;

    conversation (contact::contact_id my_id
        , contact::contact_id addressee_id
        , database_handle_t dbh
        , error * perr = nullptr);

public:
    // Construct invalid conversation
    conversation () {}
    conversation (conversation && other) = default;
    ~conversation () = default;
};

}}} // namespace chat::persistent_storage::sqlite3
