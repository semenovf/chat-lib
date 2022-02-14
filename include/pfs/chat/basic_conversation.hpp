////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.02 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include "message.hpp"

namespace chat {

template <typename Impl, typename Traits>
class basic_conversation
{
public:
    using editor_type = typename Traits::editor_type;

public:
    /**
     * Checks if message store opened/initialized successfully.
     */
    operator bool () const noexcept
    {
        return static_cast<Impl const *>(this)->ready();
    }

    /**
     * Wipes (erases all messages) conversation.
     */
    bool wipe (error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->wipe_impl(perr);
    }

    /**
     * Total number of messages in conversation.
     */
    auto count () const -> std::size_t
    {
        return static_cast<Impl const *>(this)->count_impl();
    }

    /**
     * Number of unread messages for conversation.
     */
    auto unread_messages_count () const -> std::size_t
    {
        return static_cast<Impl const *>(this)->unread_messages_count_impl();
    }

    /**
     * Creates editor for new outgoing message.
     *
     * @return Editor instance.
     */
    editor_type create (error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->create_impl(perr);
    }

    /**
     * Opens editor for outgoing message specified by @a id.
     *
     * @return Editor instance.
     */
    editor_type open (message::message_id id, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->open_impl(id, perr);
    }
};

} // namespace chat
