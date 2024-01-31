////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "file.hpp"
#include "json.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/mime.hpp"
#include "pfs/optional.hpp"
#include "pfs/time_point.hpp"
#include "pfs/universal_id.hpp"
#include <memory>
#include <utility>

namespace chat {
namespace message {

using id = ::pfs::universal_id;

class id_generator
{
public:
    id_generator () {}

    id next () noexcept
    {
        return pfs::generate_uuid();
    }
};

struct content_credentials
{
    bool is_attachment;   // Attacment flag (is content is attachment or embedded data)
    mime::mime_enum mime; // Message content MIME
    std::string text;     // Message content or file name for attachments, audio
                          // and video files, or SDP description for Live Video.
};

struct attachment_credentials
{
    file::id         file_id;
    std::string      name;    // file name
    file::filesize_t size;
};

// FrameType must be same or at least convertable to ionik::audio::wav_spectrum::unified_frame
// for stereo and ionik::audio::wav_spectrum::unified_frame::first for mono
template <typename FrameType>
struct audio_wav_credentials_basic
{
    std::uint8_t num_channels; // Number of channels: 1 - Mono, 2 - Stereo
    std::uint32_t duration;    // Duration in milliseconds

    FrameType min_frame;
    FrameType max_frame;
    std::vector<FrameType> data;
};

// using audio_wav_mono_credentials = audio_wav_credentials_basic<float>;
// using audio_wav_stereo_credentials = audio_wav_credentials_basic<std::pair<float, float>>;
using audio_wav_credentials = audio_wav_credentials_basic<std::pair<float, float>>;

struct live_video_credentials
{
    std::string description; // For SDP - SDP description if Live Video has started, or "-" if
                             // Live Video has stopped
};

class content
{
    json _d;

public:
    CHAT__EXPORT content ();

    /**
     * Construct content from JSON source.
     *
     * @throw chat::error @c errc::json_error on JSON parse error.
     */
    CHAT__EXPORT content (std::string const & source);

    CHAT__EXPORT content (content const & other);
    CHAT__EXPORT content (content && other);
    CHAT__EXPORT content & operator = (content const & other);
    CHAT__EXPORT content & operator = (content && other);
    CHAT__EXPORT ~content ();

    /**
     * Checks if content is initialized (loaded from source).
     */
    operator bool () const
    {
        return !!_d;
    }

    bool empty () const noexcept
    {
        return count() == 0;
    }

    /**
     * Number of content components.
     */
    CHAT__EXPORT std::size_t count () const noexcept;

    /**
     * Encode content to string representation
     */
    CHAT__EXPORT std::string to_string () const noexcept;

    /**
     * Returns content credentials of the component specified by @a index.
     */
    CHAT__EXPORT content_credentials at (std::size_t index) const;

    /**
     * Returns file credentials of the component specified by @a index.
     * If no attachment specified for component by @a index, result will
     * contain zeroed values for @c name, @c size.
     */
    CHAT__EXPORT attachment_credentials attachment (std::size_t index) const;

    /**
     * Returns audio WAV credentials of the component specified by @a index.
     * If no audio WAV credentials specified for component by @a index, result will
     * contain zeroed values for @c num_channels, @c duration.
     */
    CHAT__EXPORT audio_wav_credentials audio_wav (std::size_t index) const;

    /**
     * Returns Live Video credentials of the component specified by @a index.
     * If Live Video credentials specified for component by @a index, result will
     * contain empty string for @c description.
     *
     * @note Only SDP supported now.
     */
    CHAT__EXPORT live_video_credentials live_video (std::size_t index) const;

    /**
     * Add plain text.
     */
    CHAT__EXPORT void add_text (std::string const & text);

    /**
     * Add HTML text.
     */
    CHAT__EXPORT void add_html (std::string const & text);

    /**
     * Add audio WAV credentials.
     */
    CHAT__EXPORT void add_audio_wav (audio_wav_credentials const & wav
        , file::credentials const & fc);

    /**
     * Add Live Video credentials to notify Live Video has started or stopped.
     *
     * @note Only SDP supported now.
     */
    CHAT__EXPORT void add_live_video (live_video_credentials const & lvc);

    /**
     * Attach file.
     */
    CHAT__EXPORT void attach (file::credentials const & fc);

    /**
     * Clear content (delete all content components).
     */
    CHAT__EXPORT void clear ();
};

inline std::string to_string (content const & c)
{
    return c.to_string();
}

struct message_credentials
{
    // Unique message ID.
    id message_id;

    // Author contact ID.
    contact::id author_id;

    // Message creation time (UTC).
    // Creation time in the author side.
    pfs::utc_time_point creation_time;

    // Message last modification time (UTC).
    pfs::utc_time_point modification_time;

    // Delivered time (for outgoing) or received (for incoming) (UTC)
    pfs::optional<pfs::utc_time_point> delivered_time;

    // Message read time (UTC)
    pfs::optional<pfs::utc_time_point> read_time;

    pfs::optional<content> contents;
};

}} // namespace chat::message
