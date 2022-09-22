////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.07.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/error.hpp"
#include "pfs/chat/file.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/i18n.hpp"
#include "pfs/time_point.hpp"

namespace chat {
namespace file {

namespace fs = pfs::filesystem;

/**
 * Obtains file last modification time in UTC.
 *
 * @return File last modification time in UTC.
 * @throw chat::error (@c errc::filesystem_error) on filesystem error while.
 */
static pfs::utc_time_point file_modtime_utc (fs::path const & path)
{
    std::error_code ec;
    auto last_write_time = fs::last_write_time(path, ec);

    if (ec)
        throw error {errc::filesystem_error, fs::utf8_encode(path), ec.message()};

    return pfs::to_utc_time_point(last_write_time);
}

/**
 * Obtains file size with checking the upper limit.
 *
 * @return File size or @c filesize_t{-1} if the file size exceeded the limit.
 *
 * @throw chat::error (@c errc::filesystem_error) on filesystem error while.
 */
static filesize_t file_size_check_limit (fs::path const & path)
{
    std::error_code ec;
    auto filesize = fs::file_size(path, ec);

    if (ec)
        throw error {errc::filesystem_error, fs::utf8_encode(path), ec.message()};

    if (filesize > (std::numeric_limits<filesize_t>::max)())
        return filesize_t{-1};

    return static_cast<filesize_t>(filesize);
}

file::file_credentials make_credentials (fs::path const & path)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    auto utf8_path = fs::utf8_encode(abspath);
    std::string errdesc;

    if (!fs::exists(path))
        throw error {errc::attachment_failure, utf8_path, tr::_("file not found")};

    if (!fs::is_regular_file(path))
        throw error {errc::attachment_failure, utf8_path, tr::_("attachment must be a regular file")};

    auto filesize = file_size_check_limit(path);

    if (filesize < 0) {
        throw error {errc::attachment_failure, utf8_path
            , tr::_("maximum file size limit exceeded"
                ", use another way to transfer file or data")};
    }

    file_credentials res;

    res.file_id = id_generator{}.next();
    res.path    = abspath;
    res.name    = fs::utf8_encode(path.filename());
    res.size    = filesize;
    res.modtime = file_modtime_utc(path);

    return res;
}

}} // namespace chat::file
