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
#include "pfs/i18n.hpp"
#include "pfs/sha256.hpp"

namespace chat {
namespace file {

namespace fs = pfs::filesystem;

file::file_credentials make_credentials (pfs::filesystem::path const & path
    , pfs::crypto::sha256_digest const & sha256)
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

    std::error_code ec;
    auto filesize = fs::file_size(path, ec);

    if (ec)
        throw error {errc::attachment_failure, utf8_path, ec.message()};

    file_credentials res;

    res.fileid = id_generator{}.next();
    res.path   = abspath;
    res.name   = fs::utf8_encode(path.filename());
    res.size   = filesize;
    res.sha256 = sha256;

    return res;
}

file::file_credentials make_credentials (pfs::filesystem::path const & path)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    std::error_code ec;
    auto digest = pfs::crypto::sha256::digest(abspath, ec);

    if (ec) {
        throw error {errc::attachment_failure
            , fs::utf8_encode(abspath)
            , tr::f_("SHA256 generation failure: {}", ec.message())};
    }

    return make_credentials(path, digest);
}

}} // namespace chat::file
