////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.02 Initial version.
//      2022.02.17 Refactored to use backend.
//      2024.11.29 Started V2.
//                 Renamed conversation to chat.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "editor.hpp"
#include "exports.hpp"
#include "file.hpp"
#include "flags.hpp"
#include "message.hpp"
#include <pfs/optional.hpp>
#include <pfs/time_point.hpp>
#include <pfs/unicode/search.hpp>
#include <pfs/unicode/utf8_iterator.hpp>
#include <cstdint>
#include <functional>
#include <memory>

namespace chat {

enum class chat_sort_flag: int
{
      by_id                = 1 << 0
    , by_creation_time     = 1 << 1
    , by_modification_time = 1 << 2
    , by_delivered_time    = 1 << 3
    , by_read_time         = 1 << 4

    , ascending_order  = 1 << 8
    , descending_order = 1 << 9
};

template <typename Storage>
class chat final
{
    using rep = typename Storage::chat;
    using utf8_iterator = pfs::unicode::utf8_iterator<std::string::const_iterator>;

public:
    using editor_type = editor<Storage>;

private:
    std::unique_ptr<rep> _d;

public:
    /**
     * Stores attachment/file credentials for outgoing file.
     *
     * @details This callback passed to editor and used by editor only.
     *
     * @return Stored attachment/file credentials.
     *
     * @throw chat::error{errc::filesystem_error} on filesystem error.
     * @throw chat::error{errc::attachment_failure} if specific attachment error occurred.
     */
    mutable std::function<file::credentials (message::id message_id
        , std::int16_t attachment_index
        , pfs::filesystem::path const &)> cache_outgoing_local_file;

    mutable std::function<file::credentials (message::id message_id
        , std::int16_t attachment_index
        , std::string const & /*uri*/
        , std::string const & /*display_name*/
        , std::int64_t /*size*/
        , pfs::utc_time /*modtime*/)> cache_outgoing_custom_file;

public:
    CHAT__EXPORT chat ();
    CHAT__EXPORT chat (chat && other);
    CHAT__EXPORT chat & operator = (chat && other);
    CHAT__EXPORT ~chat ();

    // For internal use only
    CHAT__EXPORT chat (rep * d) noexcept;

    chat (chat const & other) = delete;
    chat & operator = (chat const & other) = delete;

public:
    /**
     * Checks if message store opened/initialized successfully.
     */
    CHAT__EXPORT operator bool () const noexcept;

    /**
     * This conversation identifier.
     */
    CHAT__EXPORT contact::id id () const noexcept;

    /**
     * Total number of messages in conversation.
     */
    CHAT__EXPORT std::size_t count () const;

    /**
     *
     * Number of unread messages for chat.
     */
    CHAT__EXPORT std::size_t unread_message_count () const;

    /**
     * Mark (if not already marked) message delivered by addressee.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    CHAT__EXPORT void mark_delivered (message::id message_id, pfs::utc_time_point delivered_time);

    /**
     * Mark (if not already marked) message received.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    void mark_received (message::id message_id, pfs::utc_time_point received_time)
    {
        mark_delivered(message_id, received_time);
    }

    /**
     * Mark (if not already marked) message read by addressee.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    CHAT__EXPORT void mark_read (message::id message_id, pfs::utc_time_point read_time);

    /**
     * Creates editor for new outgoing message.
     *
     * @return Editor instance.
     *
     * @throw debby::error on storage error.
     */
    CHAT__EXPORT editor_type create ();

    /**
     * Opens editor for outgoing message specified by @a id.
     *
     * @return Editor instance or invalid editor if message not found.
     *
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     */
    CHAT__EXPORT editor_type open (message::id id);

    /**
     * Save incoming message.
     *
     * If message already exists content will be updated if different from
     * original.
     */
    CHAT__EXPORT void save_incoming (message::id message_id, contact::id author_id
        , pfs::utc_time_point const & creation_time, std::string const & content);

    /**
     * Get message credentials by @a message_id.
     *
     * @return Message credentials or @c nullopt if message not found.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     * @throw chat::error{} if message content is invalid (i.e. bad JSON source).
     */
    CHAT__EXPORT pfs::optional<message::message_credentials> message (message::id message_id) const;

    /**
     * Get message credentials by @a offset.
     *
     * @return Message credentials or @c nullopt if message not found.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     *
     * @note By default, messages are sorted by sequential identifier. Thus the
     *       messages can be displayed in the order in which they were saved.
     */
    CHAT__EXPORT pfs::optional<message::message_credentials>
    message (int offset, int sf = sort_flags(chat_sort_flag::by_id
        , chat_sort_flag::ascending_order)) const;

    /**
     * Get last message credentials.
     *
     * @return Message credentials or @c nullopt if message not found.
     */
    CHAT__EXPORT pfs::optional<message::message_credentials> last_message () const;

    /**
     * Fetch all chat messages in order specified by @a sort_flag
     * (see @c conversation_sort_flag).
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     */
    CHAT__EXPORT void for_each (std::function<void(message::message_credentials const &)> f
        , int sort_flags, int max_count) const;

    /**
     * Convenient function for fetch all chat messages in order
     * @c conversation_sort_flag::by_creation_time | @c conversation_sort_flag::ascending_order
     */
    void for_each (std::function<void(message::message_credentials const &)> f, int max_count = -1) const
    {
        int sf = sort_flags(
              chat_sort_flag::by_creation_time
            , chat_sort_flag::ascending_order);

        for_each(f, sf, max_count);
    }

    /**
     * Erases all messages for chat.
     *
     * @throw debby::error on storage error.
     */
    CHAT__EXPORT void clear ();

    /**
     * Wipes (erases all messages) chat.
     * This call can perform a hard erasing including tables/databases.
     * Use clear() method for soft cleaning messages.
     *
     * @throw debby::error on storage error.
     */
    CHAT__EXPORT void wipe ();
};

} // namespace chat
