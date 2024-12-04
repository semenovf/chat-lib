////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.04 Initial version.
//      2022.02.17 Refactored to use backend.
//      2024.12.01 Started V2.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "exports.hpp"
#include "message.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/time_point.hpp>
#include <cstdint>
#include <functional>
#include <string>

CHAT__NAMESPACE_BEGIN

template <typename Storage>
class chat;

template <typename Storage>
class editor final
{
    template <typename B>
    friend class chat;

    using rep = typename Storage::editor;

private:
    std::unique_ptr<rep> _d;

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

public:
    CHAT__EXPORT editor (editor && other) noexcept;
    CHAT__EXPORT editor & operator = (editor && other) noexcept;
    CHAT__EXPORT ~editor ();
    CHAT__EXPORT editor (rep * d) noexcept;

    editor (editor const & other) = delete;
    editor & operator = (editor const & other) = delete;

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
};

CHAT__NAMESPACE_END
