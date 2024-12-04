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
#include <utility>

namespace chat {
namespace file {

namespace fs = pfs::filesystem;

class id_generator
{
public:
    id_generator () {}

    id next () noexcept
    {
        return pfs::generate_uuid();
    }
};

/**
 * Obtains file last modification time in UTC.
 *
 * @return File last modification time in UTC.
 * @throw chat::error { @c errc::filesystem_error } on filesystem error while.
 */
static pfs::utc_time_point modtime_utc (fs::path const & path)
{
    std::error_code ec;
    auto last_write_time = fs::last_write_time(path, ec);

    if (ec)
        throw error {errc::filesystem_error, fs::utf8_encode(path), ec.message()};

    return pfs::utc_time_point_cast(pfs::local_time_point{last_write_time.time_since_epoch()});
}

static std::pair<bool, std::size_t> file_size_check_limit (std::size_t filesize)
{
    if (filesize > (std::numeric_limits<filesize_t>::max)())
        return std::make_pair(false, filesize);

    return std::make_pair(true, filesize);
}

/**
 * Obtains file size with checking the upper limit.
 *
 * @throw chat::error (@c errc::filesystem_error) on filesystem error.
 */
static std::pair<bool, std::size_t> file_size_check_limit (fs::path const & path)
{
    std::error_code ec;
    auto filesize = fs::file_size(path, ec);

    if (ec)
        throw error {errc::filesystem_error, fs::utf8_encode(path), ec.message()};

    if (filesize > (std::numeric_limits<filesize_t>::max)())
        return std::make_pair(false, filesize);

    return std::make_pair(true, filesize);
}

static error make_filesize_limit_error (std::string const & path, std::size_t filesize)
{
    return error {
          errc::attachment_failure
        , path
        , tr::f_("maximum file size limit ({} bytes) exceeded: {} bytes"
            ", use another way to transfer file or data"
            , (std::numeric_limits<filesize_t>::max)(), filesize)
    };
}

credentials::credentials (contact::id author_id, contact::id chat_id
    , message::id message_id, std::int16_t attachment_index, fs::path const & path)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    auto utf8_path = fs::utf8_encode(abspath);

    if (!fs::exists(path))
        throw error { errc::file_not_found, utf8_path };

    if (!fs::is_regular_file(path)) {
        throw error {
              errc::attachment_failure
            , utf8_path
            , tr::_("attachment must be a regular file")
        };
    }

    auto res = file_size_check_limit(path);

    if (!res.first)
        throw make_filesize_limit_error(utf8_path, res.second);

    auto mime = mime::mime_by_extension_fallback(utf8_path);

    this->file_id          = id_generator{}.next();
    this->author_id        = author_id;
    this->chat_id          = chat_id;
    this->message_id       = message_id;
    this->attachment_index = attachment_index;
    this->abspath          = utf8_path;
    this->name             = fs::utf8_encode(path.filename());
    this->size             = static_cast<filesize_t>(res.second);
    this->mime             = mime;
    this->modtime          = modtime_utc(path);
}

credentials::credentials (contact::id author_id
    , contact::id chat_id
    , message::id message_id
    , std::int16_t attachment_index
    , std::string const & uri
    , std::string const & display_name
    , std::int64_t size
    , pfs::utc_time_point modtime)
{
    auto res = file_size_check_limit(size);

    if (!res.first)
        throw make_filesize_limit_error(uri, size);

    auto mime = mime::mime_by_extension_fallback(display_name);

    this->file_id          = id_generator{}.next();
    this->author_id        = author_id;
    this->chat_id          = chat_id;
    this->message_id       = message_id;
    this->attachment_index = attachment_index;
    this->abspath          = uri;
    this->name             = display_name;
    this->size             = static_cast<filesize_t>(size);
    this->mime             = mime;
    this->modtime          = modtime;
}

credentials::credentials (file::id file_id
    , contact::id author_id
    , contact::id chat_id
    , message::id message_id
    , std::int16_t attachment_index
    , std::string const & name
    , std::size_t size
    , mime::mime_enum mime)
{
    auto res = file_size_check_limit(size);

    if (!res.first)
        throw make_filesize_limit_error(name, size);

    this->file_id          = file_id;
    this->author_id        = author_id;
    this->chat_id          = chat_id;
    this->message_id       = message_id;
    this->attachment_index = attachment_index;
    this->abspath          = std::string{};
    this->name             = name;
    this->size             = static_cast<filesize_t>(size);
    this->mime             = mime;
    this->modtime          = pfs::utc_time_point{};
}

credentials::credentials (file::id file_id, pfs::filesystem::path const & path
    , bool no_mime)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    auto utf8_path = fs::utf8_encode(abspath);

    if (!fs::exists(path))
        throw error { errc::file_not_found, utf8_path };

    if (!fs::is_regular_file(path)) {
        throw error {
              errc::attachment_failure
            , utf8_path
            , tr::_("attachment must be a regular file")
        };
    }

    auto res = file_size_check_limit(path);

    if (!res.first)
        throw make_filesize_limit_error(utf8_path, size);

    this->file_id         = file_id;
    //this->author_id = author_id;
    //this->chat_id = chat_id;
    this->abspath         = utf8_path;
    this->name            = fs::utf8_encode(path.filename());
    this->size            = static_cast<filesize_t>(res.second);
    this->modtime         = modtime_utc(path);

    if (! no_mime)
        this->mime = mime::mime_by_extension_fallback(utf8_path);
}

}} // namespace chat::file
