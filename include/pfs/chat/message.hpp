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
#include "json.hpp"
#include "pfs/optional.hpp"
#include "pfs/time_point.hpp"
#include "pfs/universal_id.hpp"
#include <memory>

namespace chat {
namespace message {

using message_id  = ::pfs::universal_id;
using resource_id = ::pfs::universal_id;

// enum class status_enum
// {
//       draft
//     , dispatched
//     , delivered
//     , read
// };

// Media Types
// [Media Types](https://www.iana.org/assignments/media-types/media-types.xhtml)
enum class mime_enum
{
      // Used internally
      invalid

    // The default value for textual files.
    // A textual file should be human-readable and must not contain binary data.
    , text__plain

    , text__html

    // The default value for all other cases. An unknown file type should
    // use this type.
    //
    // Unrecognized subtypes of "audio" should at a miniumum be treated as
    // "application/octet-stream".
    // [RFC 2046](https://www.iana.org/assignments/media-types/audio/basic)
    , application__octet_stream

    // Special MIME type for data described by `file_credentials`
    , attachment
};

mime_enum to_mime (int value);

class id_generator
{
public:
    using type = message_id;

public:
    id_generator () {}

    type next () noexcept
    {
        return pfs::generate_uuid();
    }
};

struct content_credentials
{
    mime_enum   mime;   // Message content MIME
    std::string text;   // Message content (path for `attached`)
};

struct file_credentials
{
    std::string name;   // File name (for incoming message)/path (for outgoing message)
    std::size_t size;   // File size
    std::string sha256; // File SHA-256 checksum
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
     * contain zeroed values for @c name, @c size and @c sha256.
     */
    CHAT__EXPORT file_credentials attachment (std::size_t index) const;

    /**
     * Add component associated with @a mime.
     */
    CHAT__EXPORT void add (mime_enum mime, std::string const & data);

    /**
     * Attach file.
     */
    CHAT__EXPORT void attach (std::string const & path
        , std::size_t size
        , std::string const & sha256);
};

inline std::string to_string (content const & c)
{
    return c.to_string();
}

struct message_credentials
{
    // Unique message ID
    message_id id;

    // Author contact ID
    contact::contact_id author_id;

    // Message creation time (UTC).
    // Creation time in the author side.
    pfs::utc_time_point creation_time;

    // Message last modification time (UTC). Reserved for future use.
    pfs::utc_time_point modification_time;

    // Message dispatched time (UTC)
    pfs::optional<pfs::utc_time_point> dispatched_time;

    // Delivered time (for outgoing) or received (for incoming) (UTC)
    pfs::optional<pfs::utc_time_point> delivered_time;

    // Message read time (UTC)
    pfs::optional<pfs::utc_time_point> read_time;

    pfs::optional<content> contents;
};

struct group_message_credentials
{
    // Unique group message ID
    message_id id;

    // Author contact ID
    contact::contact_id author_id;

    // Message creation time (UTC).
    pfs::utc_time_point creation_time;

    // Message last modification time (UTC). Reserved for future use.
    pfs::utc_time_point modification_time;

    pfs::optional<content> contents;
};

struct member_message_credentials
{
    // Personal unique message ID
    message_id id;

    // Group message ID
    message_id group_id;

    // Message dispatched time (UTC)
    pfs::optional<pfs::utc_time_point> dispatched_time;

    // Delivered time (for outgoing) or received (for incoming) (UTC)
    pfs::optional<pfs::utc_time_point> delivered_time;

    // Message read time (UTC)
    pfs::optional<pfs::utc_time_point> read_time;
};

}} // namespace chat::message
