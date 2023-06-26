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
#include "mime.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/optional.hpp"
#include "pfs/time_point.hpp"
#include "pfs/universal_id.hpp"
#include <memory>

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
    mime_enum   mime; // Message content MIME
    std::string text; // Message content or file name for attachments, audio
                      // and video files
};

struct attachment_credentials
{
    file::id         file_id;
    std::string      name;    // file name
    file::filesize_t size;
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
     * Add plain text.
     */
    CHAT__EXPORT void add_text (std::string const & text);

    /**
     * Add plain text.
     */
    CHAT__EXPORT void add_html (std::string const & text);

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
