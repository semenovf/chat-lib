////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.27 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include "message.hpp"
#include "pfs/memory.hpp"

namespace chat {

template <typename Backend>
class message_store
{
    using rep_type = typename Backend::rep_type;

public:
    using conversation_type = typename Backend::conversation_type;

private:
    rep_type _rep;

private:
    message_store () = delete;
    CHAT__EXPORT message_store (rep_type && rep);
    message_store (message_store const & other) = delete;
    message_store & operator = (message_store const & other) = delete;
    message_store & operator = (message_store && other) = delete;

public:
    message_store (message_store && other) = default;
    ~message_store () = default;

public:
    /**
     * Checks if message store opened/initialized successfully.
     */
    CHAT__EXPORT operator bool () const noexcept;

    /**
     * Opens conversation with contact @a c.
     *
     * @brief This method initializes/opens data storage for conversation messages
     *        associated with specified contact.
     */
    CHAT__EXPORT conversation_type conversation (contact::contact_id addressee_id) const;

    /**
     * Wipes (erases all, drop all conversations) messages.
     *
     * @throw debby::error on storage error.
     */
    CHAT__EXPORT void wipe () noexcept;

public:
    template <typename ...Args>
    static message_store make (Args &&... args)
    {
        return message_store{Backend::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<message_store> make_unique (Args &&... args)
    {
        auto ptr = new message_store{Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<message_store>(ptr);
    }
};

} // namespace chat
