////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.04 Initial version.
//      2022.02.17 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "content_traits.hpp"
#include "pfs/bits/compiler.h"
#include "pfs/sha256.hpp"
#include "pfs/chat/editor.hpp"
#include "pfs/chat/backend/sqlite3/editor.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
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

using namespace debby::sqlite3;
namespace fs = pfs::filesystem;

namespace backend {
namespace sqlite3 {

editor::rep_type
editor::make (message::message_id message_id
    , shared_db_handle dbh
    , std::string const & table_name
    , error *)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.table_name = table_name;
    rep.message_id = message_id;
    rep.modification  = false;

    return rep;
}

editor::rep_type
editor::make (message::message_id message_id
    , message::content && content
    , shared_db_handle dbh
    , std::string const & table_name
    , error *)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.table_name = table_name;
    rep.message_id = message_id;
    rep.content    = std::move(content);
    rep.modification  = false;

    return rep;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::editor

template <>
editor<BACKEND>::editor (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
editor<BACKEND>::operator bool () const noexcept
{
    return _rep.message_id != message::message_id{};
}

template <>
void
editor<BACKEND>::add_text (std::string const & text)
{
    _rep.content.add(message::mime_enum::text__plain, text);
}

template <>
void
editor<BACKEND>::add_html (std::string const & text)
{
    _rep.content.add(message::mime_enum::text__html, text);
}

template <>
void
editor<BACKEND>::add_emoji (std::string const & shortcode)
{
    _rep.content.add(message::mime_enum::text__emoji, shortcode);
}

namespace {
    std::string const FILE_NOTFOUND_ERROR {"file not found"};
    std::string const NOT_REGULAR_FILE_ERROR {"attachment must be a regular file"};
    std::string const SHA256_GENERATION_ERROR {"SHA256 generation failure"};
}

template <>
bool
editor<BACKEND>::attach (fs::path const & path, error * perr)
{
    auto utf8_path = path.is_absolute()
        ? fs::utf8_encode(path)
        : fs::utf8_encode(fs::absolute(path));
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
                        _rep.content.attach(utf8_path, file_size, to_string(digest));
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

    error err {errc::access_attachment_failure, utf8_path, errdesc};

    if (perr)
        *perr = err;
    else
        CHAT__THROW(err);

    return false;
}

namespace {

std::string const SAVE_CONTENT {
    "UPDATE OR IGNORE `{}` SET `content` = :content"
    " WHERE `message_id` = :message_id"
};

std::string const MODIFY_CONTENT {
    "UPDATE OR IGNORE `{}` SET `content` = :content"
    ", `modification_time` = :modification_time"
    " WHERE `message_id` = :message_id"
};

} // namespace

template <>
bool
editor<BACKEND>::save (error * perr)
{
    debby::error storage_err;
    std::string const * sql = _rep.modification ? & MODIFY_CONTENT : & SAVE_CONTENT;
    auto stmt = _rep.dbh->prepare(fmt::format(*sql, _rep.table_name)
        , true, & storage_err);
    bool success = !!stmt;

    success = success
        && stmt.bind(":content"   , to_storage(_rep.content), false, & storage_err)
        && stmt.bind(":message_id", to_storage(_rep.message_id), false, & storage_err);

    if (_rep.modification) {
        auto modification_time = pfs::current_utc_time_point();
        success = success && stmt.bind(":modification_time"
            , to_storage(modification_time), & storage_err);
    }

    if (success) {
        auto res = stmt.exec(& storage_err);

        if (res.is_error())
            success = false;
    }

    if (!success) {
        error err {errc::storage_error
            , fmt::format("save content failure: #{}", to_string(_rep.message_id))
            , storage_err.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return false;
    }

    return stmt.rows_affected() > 0;
}

template <>
message::content const &
editor<BACKEND>::content () const noexcept
{
    return _rep.content;
}

template <>
message::message_id
editor<BACKEND>::message_id () const noexcept
{
    return _rep.message_id;
}

} // namespace chat
