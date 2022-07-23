////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.04 Initial version.
//      2022.02.17 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/bits/compiler.h"
#include "pfs/chat/editor.hpp"
#include "pfs/chat/file_cache.hpp"
#include "pfs/chat/backend/sqlite3/editor.hpp"
#include "pfs/i18n.hpp"
#include "pfs/sha256.hpp"
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
editor::make (message::id message_id
    , shared_db_handle dbh
    , std::string const & table_name)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.table_name = table_name;
    rep.message_id = message_id;

    return rep;
}

editor::rep_type
editor::make (message::id message_id
    , message::content && content
    , shared_db_handle dbh
    , std::string const & table_name)
{
    rep_type rep;

    rep.dbh = dbh;
    rep.table_name = table_name;
    rep.message_id = message_id;
    rep.content    = std::move(content);

    return rep;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::editor;

template <>
editor<BACKEND>::editor (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
editor<BACKEND> & editor<BACKEND>::operator = (editor && other) = default;

template <>
editor<BACKEND>::operator bool () const noexcept
{
    return _rep.message_id != message::id{};
}

template <>
void
editor<BACKEND>::add_text (std::string const & text)
{
    _rep.content.add_text(text);
}

template <>
void
editor<BACKEND>::add_html (std::string const & text)
{
    _rep.content.add_html(text);
}

template <>
void
editor<BACKEND>::attach (fs::path const & path)
{
    namespace fs = pfs::filesystem;

    auto utf8_path = path.is_absolute()
        ? fs::utf8_encode(path)
        : fs::utf8_encode(fs::absolute(path));
    std::string errdesc;

    if (!fs::exists(path))
        throw error {errc::attachment_failure, utf8_path, tr::_("file not found")};

    if (!fs::is_regular_file(path))
        throw error {errc::attachment_failure, utf8_path, tr::_("attachment must be a regular file")};

    std::error_code ec;
    auto file_size = fs::file_size(path, ec);

    if (ec)
        throw error {errc::attachment_failure, utf8_path, ec.message()};

    std::ifstream ifs {utf8_path, std::ios::binary};

    if (!ifs.is_open())
        throw error {errc::attachment_failure, utf8_path, tr::_("failed to open")};

    bool success = true;
    auto digest = pfs::crypto::sha256::digest(ifs, & success);

    using pfs::crypto::to_string;

    if (!success)
        throw error {errc::attachment_failure, utf8_path, tr::_("SHA256 generation failure")};

    auto file_id = file::id_generator{}.next();

    _rep.content.attach(file_id, fs::utf8_encode(path.filename())
        , file_size, to_string(digest));
}

template <>
void
editor<BACKEND>::clear ()
{
    _rep.content.clear();
}

template <>
message::content const &
editor<BACKEND>::content () const noexcept
{
    return _rep.content;
}

template <>
message::id
editor<BACKEND>::message_id () const noexcept
{
    return _rep.message_id;
}

} // namespace chat
