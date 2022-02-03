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
    auto wipe () -> bool
    {
        return static_cast<Impl *>(this)->wipe_impl();
    }

    /**
     * Total number of messages in conversation.
     */
    auto count () const -> std::size_t
    {
        return static_cast<Impl const *>(this)->count_impl();
    }

    /**
     * Creates editor for new outgoing message.
     *
     * @return Editor instance.
     */
    auto create (contact::contact_id addressee_id) -> editor_type
    {
        return static_cast<Impl *>(this)->create_impl(addressee_id);
    }

    /**
     * Opens editor for outgoing message specified by @a id.
     *
     * @return Editor instance.
     */
    auto open (message::message_id id) -> editor_type
    {
        return static_cast<Impl *>(this)->open_impl(id);
    }

    /**
     * Returns unread messages for conversation.
     */
    auto unread_messages_count () const -> std::size_t
    {
        return static_cast<Impl const *>(this)->unread_messages_count_impl();
    }
};

} // namespace chat
