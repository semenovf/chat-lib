////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/optional.hpp"
#include "pfs/chat/message.hpp"
#include <functional>

namespace chat {

template <typename Impl, typename Traits>
class basic_message_store
{
public:
    using conversation_type = typename Traits::conversation_type;

protected:
    basic_message_store () {}

public:
    /**
     * Checks if message store opened/initialized successfully.
     */
    operator bool () const noexcept
    {
        return static_cast<Impl const *>(this)->ready();
    }

    /**
     * Wipes (erases all) messages.
     */
    bool wipe (error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->wipe_impl(perr);
    }

    /**
     * Opens conversation with contact @a c.
     *
     * @brief This method initializes/opens data storage for conversation messages
     *        associated with specified contact.
     */
    conversation_type conversation (contact::contact_id my_id
        , contact::contact_id addressee_id) const
    {
        return static_cast<Impl const *>(this)->conversation_impl(my_id, addressee_id);
    }
};

} // namespace chat
