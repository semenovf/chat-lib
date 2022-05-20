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
#include "flags.hpp"
#include "message.hpp"
#include "pfs/optional.hpp"

namespace chat {

enum class conversation_sort_flag: int
{
      by_creation_time       = 1 << 0
    , by_modification_time   = 1 << 1
    , by_dispatched_time     = 1 << 2
    , by_delivered_time      = 1 << 3
    , by_read_time           = 1 << 4

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
    conversation (rep_type && rep);
    conversation (conversation const & other) = delete;
    conversation & operator = (conversation const & other) = delete;

public:
    conversation () = default;
    conversation (conversation && other) = default;
    conversation & operator = (conversation && other) = default;
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
    std::size_t unread_messages_count () const;

    /**
     * Mark (if not already marked) message dispatched to addressee.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    void mark_dispatched (message::message_id message_id
        , pfs::utc_time_point dispatched_time);

    /**
     * Mark (if not already marked) message delivered by addressee.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    void mark_delivered (message::message_id message_id
        , pfs::utc_time_point delivered_time);

    /**
     * Mark (if not already marked) message received.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    void mark_received (message::message_id message_id
        , pfs::utc_time_point received_time)
    {
        mark_delivered(message_id, received_time);
    }

    /**
     * Mark (if not already marked) message read by addressee.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    void mark_read (message::message_id message_id
        , pfs::utc_time_point read_time);

    /**
     * Creates editor for new outgoing message.
     *
     * @return Editor instance.
     *
     * @throw debby::error on storage error.
     */
    editor_type create ();

    /**
     * Opens editor for outgoing message specified by @a id.
     *
     * @return Editor instance or invalid editor if message not found.
     *
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     */
    editor_type open (message::message_id id);

    /**
     * Save incoming message.
     *
     * If message already exists content will be updated if different from
     * original.
     */
    void save_incoming (message::message_id message_id
        , contact::contact_id author_id
        , pfs::utc_time_point const & creation_time
        , std::string const & content);

    /**
     * Get message credentials by @a message_id.
     *
     * @return Message credentials or @c nullopt if message not found.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     */
    pfs::optional<message::message_credentials>
    message (message::message_id message_id) const;

    /**
     * Get message credentials by @a offset.
     *
     * @return Message credentials or @c nullopt if message not found.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     *
     * @note By default, messages are sorted by create time. Thus the messages
     *       can be displayed in the correct chronological order.
     */
    pfs::optional<message::message_credentials>
    message (int offset, int sf = sort_flags(conversation_sort_flag::by_creation_time
        , conversation_sort_flag::ascending_order)) const;

    /**
     * Get last message credentials.
     *
     * @return Message credentials or @c nullopt if message not found.
     */
    pfs::optional<message::message_credentials>
    last_message () const;

    /**
     * Fetch all conversation messages in order specified by @a sort_flag
     * (see @c conversation_sort_flag).
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     */
    void for_each (std::function<void(message::message_credentials const &)> f
        , int sort_flags, int max_count);

    /**
     * Convenient function for fetch all conversation messages in order
     * @c conversation_sort_flag::by_creation_time | @c conversation_sort_flag::ascending_order
     */
    void for_each (std::function<void(message::message_credentials const &)> f
        , int max_count = -1)
    {
        int sf = sort_flags(
              conversation_sort_flag::by_creation_time
            , conversation_sort_flag::ascending_order);

        for_each(f, sf, max_count);
    }

    /**
     * Erases all messages for conversation.
     *
     * @throw debby::error on storage error.
     */
    void clear ();

    /**
     * Wipes (erases all messages) conversation.
     * This call can perform a hard erasing including tables/databases.
     * Use clear() method for soft cleaning messages.
     *
     * @throw debby::error on storage error.
     */
    void wipe ();

public:
    template <typename ...Args>
    static conversation make (Args &&... args)
    {
        return conversation{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
