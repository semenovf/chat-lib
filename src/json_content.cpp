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
    char const * CONTENT_KEY     = "content";
    char const * ATTACHMENTS_KEY = "attachments";
    char const * MIME_KEY        = "mime";
    char const * TEXT_KEY        = "text";
    char const * FILE_KEY        = "file";
    char const * SIZE_KEY        = "size";
    char const * SHA256_KEY      = "sha256";
}

content::content () = default;

content::content (std::string const & source, error * perr)
{
    jeyson::error jerror;

    auto j = json::parse(source, & jerror);

    if (!j) {
        error err {errc::json_error, jerror.what()};
        if (perr) *perr = err; else CHAT__THROW(err);
        return;
    }

    if (!jeyson::is_array(j)) {
        error err {errc::json_error, "expected array"};
        if (perr) *perr = err; else CHAT__THROW(err);
        return;
    } else {
        _d = std::move(j);
    }
}

content::content (content && other) = default;
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

    auto mime_value = elem[MIME_KEY];
    assert(mime_value);
    assert(jeyson::is_integer(mime_value));

    auto x = jeyson::get<int>(mime_value);

    assert(x);
    assert(*x <= std::numeric_limits<int>::max());

    auto mime = to_mime(static_cast<int>(*x));

    switch (mime) {
        case mime_enum::text__plain:
        case mime_enum::text__html:
        case mime_enum::text_emoji:
        case mime_enum::application__octet_stream:
        case mime_enum::attachment: {
            auto key = (mime == mime_enum::attachment) ? FILE_KEY : TEXT_KEY;
            auto text_value = elem[key];
            assert(text_value);
            assert(jeyson::is_string(text_value));

            auto text = jeyson::get<std::string>(text_value);
            content_credentials result;
            return content_credentials{mime, std::string(text->c_str(), text->size())};
        }

        default:
            break;
    }

    return content_credentials{mime_enum::invalid, std::string{}};
}

void content::add (mime_enum mime, std::string const & data)
{
    switch (mime) {
        case mime_enum::text__plain:
        case mime_enum::text__html:
        case mime_enum::text_emoji:
        case mime_enum::application__octet_stream: {
            json elem;
            elem[MIME_KEY] = json{static_cast<int>(mime)};
            elem[TEXT_KEY] = json{data};
            _d.push_back(std::move(elem));
            break;
        }

        default:
            assert(false);
            break;
    }
}

void content::attach (std::string const & path
    , std::size_t size
    , std::string const & sha256)
{
    json elem;
    elem[MIME_KEY]   = json{static_cast<int>(mime_enum::attachment)};
    elem[FILE_KEY]   = json{path};
    elem[SIZE_KEY]   = json{size};
    elem[SHA256_KEY] = json{sha256};
    _d.push_back(std::move(elem));
}

std::string content::to_string () const noexcept
{
    return jeyson::to_string(_d);
}

}} // chat::message
