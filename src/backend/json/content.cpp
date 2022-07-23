////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/error.hpp"
#include "pfs/chat/message.hpp"
#include <cassert>

namespace chat {
namespace message {

namespace {
    char const * MIME_KEY   = "mime";
    char const * TEXT_KEY   = "text"; // message text or attachment path
    char const * ID_KEY     = "id";
    char const * SIZE_KEY   = "size";
    char const * SHA256_KEY = "sha256";
}

content::content () = default;

content::content (std::string const & source)
{
    json j;

    try {
        j = json::parse(source.empty() ? "[]" : source);
    } catch (jeyson::error ex) {
        throw error{errc::json_error, ex.what()};
    }

    if (!jeyson::is_array(j)) {
        throw error{errc::json_error, "expected array"};
    } else {
        _d = std::move(j);
    }
}

content::content (content const & other) = default;
content::content (content && other) = default;
content & content::operator = (content const & other) = default;
content & content::operator = (content && other) = default;
content::~content () = default;

std::size_t content::count () const noexcept
{
    return _d.size();
}

content_credentials content::at (std::size_t index) const
{
    if (index >= _d.size())
        return content_credentials{mime_enum::invalid, std::string{}};

    auto elem = _d[index];
    assert(elem);

    auto x = jeyson::get_or<int>(elem[MIME_KEY], static_cast<int>(mime_enum::invalid));
    auto mime = to_mime(static_cast<int>(x));

    switch (mime) {
        case mime_enum::text__plain:
        case mime_enum::text__html: {
            auto text = jeyson::get_or<std::string>(elem[TEXT_KEY], std::string{});
            return content_credentials{mime, text};
        }

        case mime_enum::attachment:
        case mime_enum::audio__ogg:
        case mime_enum::video__mp4: {
            auto text = jeyson::get_or<std::string>(elem[TEXT_KEY], std::string{});
            return content_credentials{mime, text};
        }

        default:
            break;
    }

    return content_credentials{mime_enum::invalid, std::string{}};
}

attachment_credentials content::attachment (std::size_t index) const
{
    if (index < _d.size()) {
        auto elem = _d[index];
        assert(elem);

        auto x = jeyson::get_or<int>(elem[MIME_KEY], static_cast<int>(mime_enum::invalid));
        auto mime = to_mime(static_cast<int>(x));

        switch (mime) {
            case mime_enum::attachment:
            case mime_enum::audio__ogg:
            case mime_enum::video__mp4: {
                auto id     = jeyson::get_or<std::string>(elem[ID_KEY], std::string{});
                auto path   = jeyson::get_or<std::string>(elem[TEXT_KEY], std::string{});
                auto size   = jeyson::get_or<std::size_t>(elem[SIZE_KEY], 0);
                auto sha256 = jeyson::get_or<std::string>(elem[SHA256_KEY], std::string{});

                return attachment_credentials {
                      std::move(id)
                    , std::move(path)
                    , size
                    , std::move(sha256)
                };
            }

            default:
                break;
        }
    }

    return attachment_credentials {
          std::string{}
        , std::string{}
        , 0
        , std::string{}
    };
}

void content::add_text (std::string const & text)
{
    json elem;
    elem[MIME_KEY] = static_cast<int>(mime_enum::text__plain);
    elem[TEXT_KEY] = text;
    _d.push_back(std::move(elem));
}

void content::add_html (std::string const & text)
{
    json elem;
    elem[MIME_KEY] = static_cast<int>(mime_enum::text__html);
    elem[TEXT_KEY] = text;
    _d.push_back(std::move(elem));
}

void content::attach (file::id file_id, std::string const & filename
    , std::size_t filesize, std::string const & sha256)
{
    using pfs::to_string;

    json elem;
    elem[MIME_KEY]   = static_cast<int>(mime_enum::attachment);
    elem[ID_KEY]     = to_string(file_id);
    elem[TEXT_KEY]   = filename;
    elem[SIZE_KEY]   = filesize;
    elem[SHA256_KEY] = sha256;
    _d.push_back(std::move(elem));
}

void content::clear ()
{
    json empty_content;
    _d.swap(empty_content);
}

std::string content::to_string () const noexcept
{
    return jeyson::to_string(_d);
}

}} // chat::message
