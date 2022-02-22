////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.02 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include "message.hpp"
#include "pfs/optional.hpp"

namespace chat {

enum conversation_sort_flag
{
      by_creation_time       = 1 << 0
    , by_local_creation_time = 1 << 1
    , by_modification_time   = 1 << 2
    , by_dispatched_time     = 1 << 3
    , by_delivered_time      = 1 << 4
    , by_read_time           = 1 << 5

    , ascending_order  = 1 << 8
    , descending_order = 1 << 9
};

template <typename B>
class message_store;

template <typename Backend>
class conversation final
{
    template <typename B>
    friend class message_store;

    using rep_type = typename Backend::rep_type;

public:
    using editor_type = typename Backend::editor_type;

private:
    rep_type _rep;

private:
    conversation () = default;
    conversation (rep_type && rep);
    conversation (conversation const & other) = delete;
    conversation & operator = (conversation const & other) = delete;
    conversation & operator = (conversation && other) = delete;

public:
    conversation (conversation && other) = default;
    ~conversation () = default;

public:
    /**
     * Checks if message store opened/initialized successfully.
     */
    operator bool () const noexcept;

    /**
     * Total number of messages in conversation.
     */
    std::size_t count () const;

    /**
     *
     * Number of unread messages for conversation.
     */
    // TODO Implement
    std::size_t unread_messages_count () const;

    /**
     * Mark message dispatched.
     */
    void mark_dispatched (message::message_id message_id
        , pfs::utc_time_point dispatched_time
        , error * perr = nullptr);

    /**
     * Creates editor for new outgoing message.
     *
     * @return Editor instance.
     */
    editor_type create (error * perr = nullptr);

    /**
     * Opens editor for outgoing message specified by @a id.
     *
     * @return Editor instance.
     */
    editor_type open (message::message_id id, error * perr = nullptr);

    pfs::optional<message::message_credentials>
    message (message::message_id message_id, error * perr = nullptr) const noexcept;

    /**
     * Fetch all conversation messages in order specified by @a sort_flag
     * (see @c conversation_sort_flag).
     *
     * @return @c true If no error occured or @c false otherwise.
     */
    bool for_each (std::function<void(message::message_credentials const &)> f
        , int sort_flag, error * perr = nullptr);

    /**
     * Convenient function for fetch all conversation messages in order
     * @c conversation_sort_flag::by_lcoal_creation_time | @c conversation_sort_flag::ascending_order
     *
     * @return @c true If no error occured or @c false otherwise.
     */
    bool for_each (std::function<void(message::message_credentials const &)> f
        , error * perr = nullptr)
    {
        return for_each(f
            , conversation_sort_flag::by_local_creation_time
                | conversation_sort_flag::ascending_order
            , perr);
    }

    /**
     * Wipes (erases all messages) conversation.
     */
    bool wipe (error * perr = nullptr) noexcept;

public:
    template <typename ...Args>
    static conversation make (Args &&... args)
    {
        return conversation{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
