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

template <typename Impl>
class basic_conversation
{
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
     * Creates new message.
     *
     * @return Valid message ID if message successfully created or
     *         invalid message ID on error.
     */
    auto create (contact::contact_id addressee_id) -> message::message_id
    {
        return static_cast<Impl *>(this)->create_impl(addressee_id);
    }
};

} // namespace chat
