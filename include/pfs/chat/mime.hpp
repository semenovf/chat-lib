////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.06.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/filesystem.hpp"

namespace chat {

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

    // Special MIME type for attachments (files) described by
    // `attachment_credentials`.
    , application__octet_stream

    // Concrete types of attachments.
    // [RFC 2046](https://www.iana.org/assignments/media-types/audio/basic)
    , audio__ogg

    , audio__wav

    , video__mp4

    , image__bmp
    , image__gif
    , image__ico  // vnd.microsoft.icon
    , image__jpeg
    , image__jpg = image__jpeg
    , image__png
    , image__tiff
};

/**
 * Read MIME from file header.
 */
CHAT__EXPORT
mime_enum read_mime (pfs::filesystem::path const & path);

/**
 * Get MIME by file extension.
 */
CHAT__EXPORT
mime_enum mime_by_extension (std::string const & filename);

} // namespace chat
