////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.04 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "exports.hpp"
#include "message.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/time_point.hpp>
#include <functional>
#include <string>

namespace chat {

template <typename Backend>
class conversation;

template <typename Backend>
class editor final
{
    template <typename B>
    friend class conversation;

    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    /**
     * Stores attachment/file credentials for outgoing file.
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

private:
    CHAT__EXPORT editor ();
    CHAT__EXPORT editor (rep_type && rep);
    editor (editor const & other) = delete;
    editor & operator = (editor const & other) = delete;

public:
    CHAT__EXPORT editor (editor && other);
    editor & operator = (editor && other) = delete;
    CHAT__EXPORT ~editor ();

public:
    /**
     * Add text to message content.
     */
    CHAT__EXPORT void add_text (std::string const & text);

    /**
     * Add HTML to message content.
     */
    CHAT__EXPORT void add_html (std::string const & text);

    /**
     * Add audio WAV to message content.
     */
    CHAT__EXPORT void add_audio_wav (pfs::filesystem::path const & path);

    /**
     * Notify Live Video started with SDP description @a sdp_desc.
     */
    CHAT__EXPORT void add_live_video_started (std::string const & sdp_desc);

    /**
     * Notify Live Video stopped.
     */
    CHAT__EXPORT void add_live_video_stopped ();

    /**
     * Add attachment to message content.
     *
     * @throw chat::error @c errc::attachment_failure.
     */
    CHAT__EXPORT void attach (pfs::filesystem::path const & path);

    /**
     * Add attachment to message content.
     *
     * @throw chat::error @c errc::attachment_failure.
     */
    CHAT__EXPORT void attach (std::string const & uri
        , std::string const & display_name
        , std::int64_t size
        , pfs::utc_time modtime);

    /**
     * Clear message content.
     */
    CHAT__EXPORT void clear ();

    /**
     * Save content.
     */
    CHAT__EXPORT void save ();

    /**
     * Get message content.
     */
    CHAT__EXPORT message::content const & content () const noexcept;

    /**
     * Get message ID.
     */
    CHAT__EXPORT message::id message_id () const noexcept;

private:
    template <typename ...Args>
    static editor make (Args &&... args)
    {
        return editor{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
