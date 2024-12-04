////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.12.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "chat_impl.hpp"
#include "chat/editor_mode.hpp"
#include "chat/message.hpp"
#include "chat/sqlite3.hpp"

namespace chat {
namespace storage {

class sqlite3::editor
{
public:
    chat * holder {nullptr};
    message::id      message_id;
    message::content content;
    editor_mode      mode;

public:
    editor (chat * a_holder, message::id a_message_id, message::content && a_content, editor_mode a_mode)
        : holder(a_holder)
        , message_id(a_message_id)
        , content(std::move(a_content))
        , mode(a_mode)
    {}

    editor (chat * a_holder, message::id a_message_id, editor_mode a_mode)
        : holder(a_holder)
        , message_id(a_message_id)
        , mode(a_mode)
    {}
};

}} // namespace chat::storage
