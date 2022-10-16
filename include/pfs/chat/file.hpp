////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.07.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/optional.hpp"
#include "pfs/universal_id.hpp"
#include "pfs/time_point.hpp"
#include <memory>

namespace chat {
namespace file {

using id = pfs::universal_id;
using filesize_t = std::int32_t;

class id_generator
{
public:
    id_generator () {}

    id next () noexcept
    {
        return pfs::generate_uuid();
    }
};

struct file_credentials
{
    // Unique ID associated with file name
    id file_id;

    // Absolute path.
    // For outgoing file it is absolute path in local filesystem.
    // For incoming file it is absolute path including file cache base directory.
    // It is assumed that `path` is also unique as `file_id`.
    pfs::filesystem::path path;

    // File name for attachments, audio and video files
    // For outgoing file it is same as `path.filename()`.
    // For incoming file it is original (remote) filename.
    std::string name;

    // File size
    filesize_t size;

    // File last modification time in UTC.
    pfs::utc_time_point modtime;
};

using optional_file_credentials = pfs::optional<file::file_credentials>;

/**
 * Makes file credentials.
 */
CHAT__EXPORT
file::file_credentials make_credentials (pfs::filesystem::path const & path);

}} // namespace chat::file
