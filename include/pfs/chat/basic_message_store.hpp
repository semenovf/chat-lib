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
protected:
    using failure_handler_type = std::function<void(std::string const &)>;

public:
    using conversation_type = typename Traits::conversation_type;

protected:
    failure_handler_type on_failure;

protected:
    basic_message_store (failure_handler_type f)
        : on_failure(f)
    {}

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
    auto wipe () -> bool
    {
        return static_cast<Impl *>(this)->wipe_impl();
    }

    /**
     * Begins conversation with contact @a c.
     *
     * @brief This method initializes/opens data storage for conversation messages
     * associated with specified contact.
     */
    auto begin_conversation (contact::contact_id c) -> conversation_type &
    {
        return static_cast<Impl *>(this)->begin_conversation_impl(c);
    }
};

} // namespace chat
