////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.27 Initial version.
//      2022.02.17 Refactored to use backend.
//      2024.11.30 Started V2.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "chat.hpp"
#include "exports.hpp"
#include "message.hpp"
#include <memory>

namespace chat {

template <typename Storage>
class message_store final
{
    using rep = typename Storage::message_store;

public:
    using chat_type = chat<Storage>;

private:
    std::unique_ptr<rep> _d;

private:
    CHAT__EXPORT message_store (rep * d) noexcept;

public:
    CHAT__EXPORT message_store (message_store && other) noexcept;
    CHAT__EXPORT message_store & operator = (message_store && other) noexcept;
    CHAT__EXPORT ~message_store ();

    message_store (message_store const & other) = delete;
    message_store & operator = (message_store const & other) = delete;

public:
    /**
     * Checks if message store opened/initialized successfully.
     */
    CHAT__EXPORT operator bool () const noexcept;

    /**
     * Opens conversation by @a chat_id.
     *
     * @brief This method initializes/opens data storage for conversation messages
     *        associated with specified contact.
     */
    CHAT__EXPORT chat_type open_chat (contact::id chat_id) const;

    /**
     * Clear all chats.
     */
    CHAT__EXPORT void clear () noexcept;

public:
    template <typename ...Args>
    static message_store make (Args &&... args)
    {
        return message_store{Storage::make_message_store(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<message_store> make_unique (Args &&... args)
    {
        return std::make_unique<message_store>(Storage::make_message_store(std::forward<Args>(args)...));
    }
};

} // namespace chat
