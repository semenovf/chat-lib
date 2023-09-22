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
        return content_credentials{mime::mime_enum::unknown, std::string{}};

    auto elem = _d[index];
    assert(elem);

    auto x = jeyson::get_or<int>(elem[MIME_KEY], static_cast<int>(mime::mime_enum::unknown));
    auto mime = static_cast<mime::mime_enum>(x);

    if (is_valid(mime)) {
        auto text = jeyson::get_or<std::string>(elem[TEXT_KEY], std::string{});
        return content_credentials{mime, text};
    }

    return content_credentials{mime::mime_enum::unknown, std::string{}};
}

attachment_credentials content::attachment (std::size_t index) const
{
    if (index < _d.size()) {
        auto elem = _d[index];
        assert(elem);

        auto x = jeyson::get_or<int>(elem[MIME_KEY], static_cast<int>(mime::mime_enum::unknown));
        auto mime = static_cast<mime::mime_enum>(x);

        if (is_valid(mime) && !(mime == mime::mime_enum::text__plain
                || mime == mime::mime_enum::text__html)) {
            auto file_id = jeyson::get_or<std::string>(elem[ID_KEY], std::string{});
            auto name    = jeyson::get_or<std::string>(elem[TEXT_KEY], std::string{});
            auto size    = jeyson::get_or<file::filesize_t>(elem[SIZE_KEY], 0);

            return attachment_credentials {
                    pfs::from_string<file::id>(file_id)
                , name
                , size
            };
        }
    }

    return attachment_credentials{};
}

void content::add_text (std::string const & text)
{
    json elem;
    elem[MIME_KEY] = static_cast<int>(mime::mime_enum::text__plain);
    elem[TEXT_KEY] = text;
    _d.push_back(std::move(elem));
}

void content::add_html (std::string const & text)
{
    json elem;
    elem[MIME_KEY] = static_cast<int>(mime::mime_enum::text__html);
    elem[TEXT_KEY] = text;
    _d.push_back(std::move(elem));
}

void content::attach (file::credentials const & fc)
{
    using pfs::to_string;

    json elem;
    elem[MIME_KEY]   = static_cast<int>(fc.mime);
    elem[ID_KEY]     = to_string(fc.file_id);
    elem[TEXT_KEY]   = fc.name;
    elem[SIZE_KEY]   = fc.size;
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
