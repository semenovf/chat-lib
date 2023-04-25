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
#include "editor.hpp"
#include "exports.hpp"
#include "flags.hpp"
#include "message.hpp"
#include "pfs/optional.hpp"
#include "pfs/time_point.hpp"
#include "pfs/unicode/search.hpp"
#include "pfs/unicode/utf8_iterator.hpp"
#include <functional>

namespace chat {

enum class conversation_sort_flag: int
{
      by_creation_time       = 1 << 0
    , by_modification_time   = 1 << 1
    , by_delivered_time      = 1 << 2
    , by_read_time           = 1 << 3

    , ascending_order  = 1 << 8
    , descending_order = 1 << 9
};

template <typename Backend>
class conversation final
{
    using rep_type = typename Backend::rep_type;
    using utf8_input_iterator = pfs::unicode::utf8_input_iterator<std::string::const_iterator>;

public:
    using editor_type = editor<typename Backend::editor_type>;

private:
    rep_type _rep;

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
    mutable std::function<file::file_credentials (pfs::filesystem::path const &)> cache_outcome_local_file;

    mutable std::function<file::file_credentials (std::string const & /*uri*/
        , std::string const & /*display_name*/
        , std::int64_t /*size*/
        , pfs::utc_time /*modtime*/)> cache_outcome_custom_file;

private:
    CHAT__EXPORT conversation (rep_type && rep);
    conversation (conversation const & other) = delete;
    conversation & operator = (conversation const & other) = delete;

public:
    CHAT__EXPORT conversation ();
    CHAT__EXPORT conversation (conversation && other);
    CHAT__EXPORT conversation & operator = (conversation && other);
    CHAT__EXPORT ~conversation ();

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
     * Number of unread messages for conversation.
     */
    CHAT__EXPORT std::size_t unread_messages_count () const;

    /**
     * Mark (if not already marked) message delivered by addressee.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    CHAT__EXPORT void mark_delivered (message::id message_id
        , pfs::utc_time_point delivered_time);

    /**
     * Mark (if not already marked) message received.
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message not found.
     */
    void mark_received (message::id message_id
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
    CHAT__EXPORT void mark_read (message::id message_id
        , pfs::utc_time_point read_time);

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
    CHAT__EXPORT void save_incoming (message::id message_id
        , contact::id author_id
        , pfs::utc_time_point const & creation_time
        , std::string const & content);

    /**
     * Get message credentials by @a message_id.
     *
     * @return Message credentials or @c nullopt if message not found.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     * @throw chat::error{} if message content is invalid (i.e. bad JSON source).
     */
    CHAT__EXPORT pfs::optional<message::message_credentials>
    message (message::id message_id) const;

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
    CHAT__EXPORT pfs::optional<message::message_credentials>
    message (int offset, int sf = sort_flags(conversation_sort_flag::by_creation_time
        , conversation_sort_flag::ascending_order)) const;

    /**
     * Get last message credentials.
     *
     * @return Message credentials or @c nullopt if message not found.
     */
    CHAT__EXPORT pfs::optional<message::message_credentials>
    last_message () const;

    /**
     * Fetch all conversation messages in order specified by @a sort_flag
     * (see @c conversation_sort_flag).
     *
     * @throw debby::error on storage error.
     * @throw chat::error if message content is invalid (i.e. bad JSON source).
     */
    CHAT__EXPORT void for_each (std::function<void(message::message_credentials const &)> f
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
    CHAT__EXPORT void clear ();

    /**
     * Wipes (erases all messages) conversation.
     * This call can perform a hard erasing including tables/databases.
     * Use clear() method for soft cleaning messages.
     *
     * @throw debby::error on storage error.
     */
    CHAT__EXPORT void wipe ();

public:
    ////////////////////////////////////////////////////////////////////////////
    // Search specific types and methods.
    ////////////////////////////////////////////////////////////////////////////
    enum search_flag
    {
          ignore_case_flag     = 1 << 0
        , text_content_flag    = 1 << 1
        , attachment_name_flag = 1 << 2
    };

    struct match_item
    {
        message::id message_id;
        pfs::unicode::match_item m;
        int content_index; // index of content in message_credentials::contents
    };

    struct search_result
    {
        std::vector<match_item> m;
    };

public:
    /**
     * Searches conversion messages for specified @a pattern.
     */
    search_result search_all (std::string const & pattern
        , int search_flags = ignore_case_flag | text_content_flag)
    {
        search_result res;

        for_each([& res, & pattern, search_flags] (message::message_credentials const & mc) {
            if (mc.contents) {
                for (int i = 0, count = mc.contents->count(); i < count; i++) {
                    auto cc = mc.contents->at(i);

                    bool is_text_content = cc.mime == message::mime_enum::text__plain
                        || cc.mime == message::mime_enum::text__html;

                    bool content_search_requested = is_text_content
                        ? (search_flags & text_content_flag)
                        : (search_flags & attachment_name_flag);

                    auto first = utf8_input_iterator::begin(cc.text.begin(), cc.text.end());
                    auto s_first = utf8_input_iterator::begin(pattern.begin(), pattern.end());

                    if (content_search_requested) {
                        if (cc.mime == message::mime_enum::text__html) {
                            pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
                                , search_flags & ignore_case_flag, '<', '>'
                                , [& res, & mc, i] (pfs::unicode::match_item const & m) {
                                    res.m.emplace_back(match_item{mc.message_id, m, i});
                                });
                        } else {
                            pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
                                , search_flags & ignore_case_flag
                                , [& res, & mc, i] (pfs::unicode::match_item const & m) {
                                    res.m.emplace_back(match_item{mc.message_id, m, i});
                                });
                        }
                    }
                }
            }
        });

        return res;
    }
public:
    template <typename ...Args>
    static conversation make (Args &&... args)
    {
        return conversation{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
