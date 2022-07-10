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

namespace fs = pfs::filesystem;

namespace backend {
namespace sqlite3 {

editor::rep_type
editor::make (message::message_id message_id
    , shared_db_handle dbh
    , std::string const & table_name)
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
    , std::string const & table_name)
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

using BACKEND = backend::sqlite3::editor;

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

namespace {
    std::string const FILE_NOTFOUND_ERROR {"file not found"};
    std::string const NOT_REGULAR_FILE_ERROR {"attachment must be a regular file"};
    std::string const SHA256_GENERATION_ERROR {"SHA256 generation failure"};
}

template <>
void
editor<BACKEND>::attach (fs::path const & path)
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
                    CHAT__ASSERT(file_size != std::ifstream::pos_type{-1}, "");

                    auto digest = pfs::crypto::sha256::digest(ifs, & success);

                    if (success) {
                        _rep.content.attach(utf8_path, file_size, to_string(digest));
                        return;
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

    error err {
          errc::attachment_failure
        , utf8_path
        , errdesc
    };

    CHAT__THROW(err);
}

static std::string const SAVE_CONTENT {
    "UPDATE OR IGNORE `{}` SET `content` = :content"
    " WHERE `message_id` = :message_id"
};

static std::string const MODIFY_CONTENT {
    "UPDATE OR IGNORE `{}` SET `content` = :content"
    ", `modification_time` = :modification_time"
    " WHERE `message_id` = :message_id"
};

template <>
void
editor<BACKEND>::save ()
{
    std::string const * sql = _rep.modification ? & MODIFY_CONTENT : & SAVE_CONTENT;
    auto stmt = _rep.dbh->prepare(fmt::format(*sql, _rep.table_name));

    CHAT__ASSERT(!!stmt, "");

    auto now = pfs::current_utc_time_point();

    stmt.bind(":content", _rep.content);
    stmt.bind(":message_id", _rep.message_id);

    if (_rep.modification)
        stmt.bind(":modification_time", now);

    stmt.exec();
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
