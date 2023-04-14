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
#include <cctype>
#include <map>

#if _MSC_VER
#   include <sys/types.h>
#   include <fcntl.h>
#else
#   include <sys/types.h>
#   include <fcntl.h>
#   include <unistd.h>
#endif

namespace chat {
namespace message {

namespace fs = pfs::filesystem;
using string_view = pfs::string_view;

// static char const MIME_DOC [] = { 208, 207, 17, 224, 161, 177, 26, 225 };
// static char const MIME_EXE_DLL [] = { 77, 90 };
// static char const MIME_MP3 [] = { 255, 251, 48 };
// static char const MIME_PDF [] = { 37, 80, 68, 70, 45, 49, 46 };
// static char const MIME_RAR [] = { 82, 97, 114, 33, 26, 7, 0 };
// static char const MIME_SWF [] = { 70, 87, 83 };
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

    std::array<item, 8> _d = {
          item { mime_enum::audio__ogg , std::string{ 79, 103, 103, 83, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 }}
        , item { mime_enum::audio__wav , std::string{ 82, 73, 70, 70 }}
        , item { mime_enum::image__bmp , std::string{ 66, 77 }}
        , item { mime_enum::image__gif , std::string{ 71, 73, 70, 56 }}
        , item { mime_enum::image__ico , std::string{ 0, 0, 1, 0 }}
        , item { mime_enum::image__jpg , std::string{ static_cast<char>(255), static_cast<char>(216), static_cast<char>(255) }}
        , item { mime_enum::image__png , std::string{ static_cast<char>(137), 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82 }}
        , item { mime_enum::image__tiff, std::string{ 73, 73, 42, 0 }}
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

mime_enum mime_by_extension (std::string const & filename)
{
    static std::map<std::string, mime_enum> __well_known_extensions {
          { "txt" , mime_enum::text__plain }
        , { "log" , mime_enum::text__plain }
        , { "html", mime_enum::text__html  }
        , { "htm" , mime_enum::text__html  }
        , { "ogg" , mime_enum::audio__ogg  } // Ogg Vorbis Audio File
        , { "wav" , mime_enum::audio__wav  } // WAVE Audio File
        , { "mp4" , mime_enum::video__mp4  } // MPEG-4 Video
        , { "bmp" , mime_enum::image__bmp  } // Bitmap Image
        , { "gif" , mime_enum::image__gif  } // Graphical Interchange Format File
        , { "ico" , mime_enum::image__ico  }
        , { "jpeg", mime_enum::image__jpeg }
        , { "jpg" , mime_enum::image__jpeg }
        , { "png" , mime_enum::image__png  }
        , { "tiff", mime_enum::image__tiff }
        , { "tif" , mime_enum::image__tiff }
    };

    if (filename.empty())
        return mime_enum::invalid;

    auto index = filename.rfind(".");

    if (index != std::string::npos) {
        auto ext = filename.substr(index + 1);

        std::transform(ext.begin(), ext.end(), ext.begin(), [] (char ch) {
            return (ch <= 'Z' && ch >= 'A') ? ch - ('Z' - 'z') : ch;
        });

        auto pos = __well_known_extensions.find(ext);

        if (pos != __well_known_extensions.end())
            return pos->second;
    }

    return mime_enum::application__octet_stream;
}

}} // namespace chat::message
