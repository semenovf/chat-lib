////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.07.23 Initial version.
//      2023.06.22 Added `author_id`, `conversation_id` and `mime` fields
//                 into credentials.
//      2023.06.23 Added constructors to `credentials` instead of make functions.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "exports.hpp"
#include "mime.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/optional.hpp"
#include "pfs/universal_id.hpp"
#include "pfs/time_point.hpp"
#include <memory>

namespace chat {
namespace file {

using id = pfs::universal_id;
using filesize_t = std::int32_t;

struct credentials
{
    // Unique ID associated with file name
    id file_id;

    contact::id author_id;
    contact::id conversation_id;

    // Absolute path.
    // For outgoing file it is absolute path in local filesystem or URI.
    // For incoming file it is absolute path including file cache base directory.
    // It is assumed that `path` is also unique as `file_id`.
    //
    std::string abspath;

    // File name for attachments, audio and video files
    // For outgoing file it is same as `path.filename()`.
    // For incoming file it is original (remote) filename.
    std::string name;

    // File size
    filesize_t size;

    mime_enum mime = mime_enum::invalid;

    // File last modification time in UTC.
    pfs::utc_time_point modtime;

public:
    credentials () = default;

    /**
     * Constracts complete file credentials from local file with new unique
     * identifier.
     *
     * @throw chat::error { @c errc::file_not_found } if file @a path not found.
     * @throw chat::error { @c errc::attachment_failure } if file @path is not a
     *        regular file or maximum file size limit exceeded.
     */
    CHAT__EXPORT
    credentials (contact::id author_id, contact::id conversation_id
        , pfs::filesystem::path const & path);

    /**
     * Constracts complete file credentials from URI (useful on Android) with
     * new unique identifier.
     */
    CHAT__EXPORT
    credentials (contact::id author_id
        , contact::id conversation_id
        , std::string const & uri
        , std::string const & display_name
        , std::int64_t size
        , pfs::utc_time_point modtime);

    /**
     * Constracts incomplete file credentials: abspath and modtime stay invalid.
     */
    CHAT__EXPORT
    credentials (file::id file_id
        , contact::id author_id
        , contact::id conversation_id
        , std::string const & name
        , std::size_t size
        , mime_enum mime);

    /**
     * Constracts incomplete file credentials: author_id and conversation_id stay
     * invalid; mime also stay invalid if @no_mime is @c true.
     */
    CHAT__EXPORT
    credentials (file::id file_id, pfs::filesystem::path const & abspath, bool no_mime);
};

using optional_credentials = pfs::optional<credentials>;

}} // namespace chat::file
