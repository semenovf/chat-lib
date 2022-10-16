////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/i18n.hpp"
#include "pfs/string_view.hpp"
#include "pfs/chat/message.hpp"
#include <algorithm>

#if _MSC_VER
#   include "windows.hpp"
#else
#   include <sys/types.h>
#   include <fcntl.h>
#   include <unistd.h>
#endif

namespace chat {
namespace message {

namespace fs = pfs::filesystem;
using string_view = pfs::string_view;

// static char const MIME_BMP [] = { 66, 77 };
// static char const MIME_DOC [] = { 208, 207, 17, 224, 161, 177, 26, 225 };
// static char const MIME_EXE_DLL [] = { 77, 90 };
// static char const MIME_GIF [] = { 71, 73, 70, 56 };
// static char const MIME_ICO [] = { 0, 0, 1, 0 };
// static char const MIME_JPG [] = { 255, 216, 255 };
// static char const MIME_MP3 [] = { 255, 251, 48 };
// static char const MIME_PDF [] = { 37, 80, 68, 70, 45, 49, 46 };
// static char const MIME_PNG [] = { 137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82 };
// static char const MIME_RAR [] = { 82, 97, 114, 33, 26, 7, 0 };
// static char const MIME_SWF [] = { 70, 87, 83 };
// static char const MIME_TIFF [] = { 73, 73, 42, 0 };
// static char const MIME_TORRENT [] = { 100, 56, 58, 97, 110, 110, 111, 117, 110, 99, 101 };
// static char const MIME_TTF [] = { 0, 1, 0, 0, 0 };
// static char const MIME_WMV_WMA [] = { 48, 38, 178, 117, 142, 102, 207, 17, 166, 217, 0, 170, 0, 98, 206, 108 };
// static char const MIME_ZIP_DOCX [] = { 80, 75, 3, 4 };

class mime_mapping
{
    struct item
    {
        mime_enum mime;
        std::string header;
    };

    std::array<item, 2> _d = {
          item { mime_enum::audio__ogg, std::string{ 79, 103, 103, 83, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 }}
        , item { mime_enum::audio__wav, std::string{ 82, 73, 70, 70 }}
    };

    std::size_t _maxlen = 0;

public:
    mime_mapping ()
    {
        for (auto const & elem: _d)
            _maxlen = (std::max)(_maxlen, elem.header.size());
    }

    std::size_t maxlen () const noexcept
    {
        return _maxlen;
    }

    mime_enum get_mime (string_view header) const noexcept
    {
        for (auto const & elem: _d) {
            if (pfs::starts_with(header, elem.header))
                return elem.mime;
        }

        return mime_enum::application__octet_stream;
    }
};

static mime_mapping const __mime_mapping;

mime_enum read_mime (pfs::filesystem::path const & path)
{
    std::error_code ec;

    if (!fs::exists(path, ec)) {
        throw error {
              make_error_code(errc::file_not_found)
            , fs::utf8_encode(path)
        };
    }

    if (!fs::is_regular_file(path, ec)) {
        throw error {
              make_error_code(errc::filesystem_error)
            , fs::utf8_encode(path)
            , tr::_("expected regular file")
        };
    }

    auto h = ::open(fs::utf8_encode(path).c_str(), O_RDONLY);

    if (h < 0)
        throw error {std::error_code(errno, std::generic_category())};

    std::vector<char> buffer(__mime_mapping.maxlen(), 0);

    auto n = ::read(h, buffer.data(), buffer.size());

    if (n < 0) {
        throw error {
              std::error_code(errno, std::generic_category())
            , tr::_("read file header")
        };
    }

    if (n == 0)
        return mime_enum::application__octet_stream;

    return __mime_mapping.get_mime(string_view{buffer.data(), static_cast<std::size_t>(n)});
}

}} // namespace chat::message
