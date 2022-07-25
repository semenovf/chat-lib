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
#include <fstream>

namespace chat {
namespace file {

namespace fs = pfs::filesystem;

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

    file_credentials res;

    res.file_id = id_generator{}.next();
    res.path    = abspath;
    res.name    = fs::utf8_encode(path.filename());
    res.size    = file_size;
    res.sha256  = to_string(digest);

    return res;
}

}} // namespace chat::file
