////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "content_traits.hpp"
#include "pfs/bits/compiler.h"
#include "pfs/sha256.hpp"
#include "pfs/chat/persistent_storage/sqlite3/editor.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <fstream>

#if PFS_COMPILER_MSVC
#   include <io.h>
#   include <fcntl.h>
#   include <share.h>
#endif // PFS_COMPILER_MSVC

#if PFS_COMPILER_GCC
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <string.h>
#endif // PFS_COMPILER_GCC

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;
namespace fs = pfs::filesystem;

editor::editor (message::message_id message_id
    , database_handle_t dbh
    , std::string const & table_name)
    : base_class()
    , _dbh(dbh)
    , _table_name(table_name)
    , _message_id(message_id)
{}

editor::editor (message::message_id message_id
    , message::content && content
    , database_handle_t dbh
    , std::string const & table_name)
    : base_class()
    , _dbh(dbh)
    , _table_name(table_name)
    , _message_id(message_id)
    , _content(std::move(content))
{}

void editor::add_text_impl (std::string const & text)
{
    _content.add(message::mime_enum::text__plain, text);
}

void editor::add_html_impl (std::string const & text)
{
    _content.add(message::mime_enum::text__html, text);
}

void editor::add_emoji_impl (std::string const & shortcode)
{
//     if (!emoji_db::has(shortcode)) {
//         auto err = error{errc::bad_emoji_shortcode, shortcode};
//         if (perr) *perr = err; else CHAT__THROW(err);
//         return false;
//     }

    _content.add(message::mime_enum::text_emoji, shortcode);
}

namespace {
    std::string const FILE_NOTFOUND_ERROR {"file not found"};
    std::string const NOT_REGULAR_FILE_ERROR {"attachment must be a regular file"};
    std::string const SHA256_GENERATION_ERROR {"SHA256 generation failure"};
}

bool editor::attach_impl (fs::path const & path, error * perr)
{
    auto utf8_path = fs::utf8_encode(path);
    std::string errdesc;

    if (fs::exists(path)) {
        if (fs::is_regular_file(path)) {
            std::ifstream ifs {utf8_path, std::ios::binary};

            if (ifs.is_open()) {
                std::ifstream::pos_type file_size;
                bool success = true;

                try {
                    ifs.seekg(0, ifs.end);
                    file_size = ifs.tellg();
                    ifs.seekg (0, ifs.beg);
                } catch (std::ios_base::failure const & ex) {
                    errdesc = ex.what();
                    success = false;
                }

                if (success) {
                    assert(file_size != std::ifstream::pos_type{-1});

                    auto digest = pfs::crypto::sha256::digest(ifs, & success);

                    if (success) {
                        _content.attach(utf8_path, file_size, to_string(digest));
                        return true;
                    } else {
                        errdesc = SHA256_GENERATION_ERROR;
                    }
                }
            }
        } else {
            errdesc = NOT_REGULAR_FILE_ERROR;
        }
    } else {
        errdesc = FILE_NOTFOUND_ERROR;
    }

    auto err = error{errc::access_attachment_failure, utf8_path, errdesc};

    if (perr)
        *perr = err;
    else
        CHAT__THROW(err);

    return false;
}

namespace {

std::string const UPDATE_CONTENT {
    "UPDATE OR IGNORE `{}` SET `content` = :content WHERE `message_id` = :message_id"
};

} // namespace

auto editor::save_impl (error * perr) -> bool
{
    debby::error storage_err;
    auto stmt = _dbh->prepare(fmt::format(UPDATE_CONTENT, _table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":content"   , to_storage(_content), false, & storage_err)
        && stmt.bind(":message_id", to_storage(_message_id), false, & storage_err);

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        auto err = error{errc::storage_error
            , fmt::format("save content failure: #{}", to_string(_message_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    return stmt.rows_affected() > 0;
}

message::content const & editor::content_impl () const noexcept
{
    return _content;
}

message::message_id editor::message_id_impl () const noexcept
{
    return _message_id;
}

}}} // namespace chat::persistent_storage::sqlite3
