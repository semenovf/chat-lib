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

static char const * ATT_KEY  = "att";  // Attacment flag (is content is attachment or embedded data)
static char const * MIME_KEY = "mime";
static char const * TEXT_KEY = "text"; // message text or attachment path
static char const * ID_KEY   = "id";
static char const * SIZE_KEY = "size";

static char const * AU_WAV_KEY       = "au-wav"; // Audio WAV subkey
static char const * AU_DURATION_KEY  = "duration"; // Duration for embedded audio or video
static char const * AU_NUM_CHAN_KEY  = "num-chan"; // Number of channels (1 or 2)
static char const * AU_MAX_FRAME_KEY = "max-frame";
static char const * AU_MIN_FRAME_KEY = "min-frame";
static char const * AU_SPECTRUM      = "spectrum";

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
    if (index < _d.size()) {
        auto elem = _d[index];
        assert(elem);

        auto att = jeyson::get_or<bool>(elem[ATT_KEY], false);
        auto x = jeyson::get_or<int>(elem[MIME_KEY], static_cast<int>(mime::mime_enum::unknown));
        auto mime = static_cast<mime::mime_enum>(x);

        if (is_valid(mime)) {
            auto text = jeyson::get_or<std::string>(elem[TEXT_KEY], std::string{});
            return content_credentials{att, mime, text};
        }
    }

    return content_credentials{false, mime::mime_enum::unknown, std::string{}};
}

attachment_credentials content::attachment (std::size_t index) const
{
    if (index < _d.size()) {
        auto elem = _d[index];
        assert(elem);

        auto att = jeyson::get_or<bool>(elem[ATT_KEY], false);
        auto x = jeyson::get_or<int>(elem[MIME_KEY], static_cast<int>(mime::mime_enum::unknown));
        auto mime = static_cast<mime::mime_enum>(x);

        if (att && is_valid(mime)) {
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

audio_wav_credentials content::audio_wav (std::size_t index) const
{
    if (index < _d.size()) {
        auto elem = _d[index];
        assert(elem);

        auto att = jeyson::get_or<bool>(elem[ATT_KEY], false);
        auto x = jeyson::get_or<int>(elem[MIME_KEY], static_cast<int>(mime::mime_enum::unknown));
        auto mime = static_cast<mime::mime_enum>(x);

        if (att && mime == mime::mime_enum::audio__wav) {
            auto num_channels = jeyson::get_or<std::uint8_t>(elem[AU_WAV_KEY][AU_NUM_CHAN_KEY], 0);
            auto duration = jeyson::get_or<std::uint32_t>(elem[AU_WAV_KEY][AU_DURATION_KEY], 0);
            auto min_frame_left  = jeyson::get_or<float>(elem[AU_WAV_KEY][AU_MIN_FRAME_KEY][0], .0f);
            auto max_frame_left  = jeyson::get_or<float>(elem[AU_WAV_KEY][AU_MAX_FRAME_KEY][0], .0f);
            auto min_frame_right = jeyson::get_or<float>(elem[AU_WAV_KEY][AU_MIN_FRAME_KEY][1], .0f);
            auto max_frame_right = jeyson::get_or<float>(elem[AU_WAV_KEY][AU_MAX_FRAME_KEY][1], .0f);

            std::vector<std::pair<float, float>> data;

            elem[AU_WAV_KEY][AU_SPECTRUM].for_each ([& data] (json::reference ref) {
                auto frame_left = jeyson::get_or<float>(ref[0], 0.f);
                auto frame_right = jeyson::get_or<float>(ref[1], 0.f);

                data.push_back(std::make_pair(frame_left, frame_right));
            });

            return audio_wav_credentials {
                  num_channels
                , duration
                , std::make_pair(min_frame_left, min_frame_right)
                , std::make_pair(max_frame_left, max_frame_right)
                , std::move(data)
            };
        }
    }

    return audio_wav_credentials{};
}

void content::add_text (std::string const & text)
{
    json elem;
    elem[ATT_KEY] = false;
    elem[MIME_KEY] = static_cast<int>(mime::mime_enum::text__plain);
    elem[TEXT_KEY] = text;
    _d.push_back(std::move(elem));
}

void content::add_html (std::string const & text)
{
    json elem;
    elem[ATT_KEY] = false;
    elem[MIME_KEY] = static_cast<int>(mime::mime_enum::text__html);
    elem[TEXT_KEY] = text;
    _d.push_back(std::move(elem));
}

static void init_attachment (json & elem, file::credentials const & fc)
{
    using pfs::to_string;

    elem[ATT_KEY]  = true;
    elem[MIME_KEY] = static_cast<int>(fc.mime);
    elem[ID_KEY]   = to_string(fc.file_id);
    elem[TEXT_KEY] = fc.name;
    elem[SIZE_KEY] = fc.size;
}

void content::add_audio_wav (audio_wav_credentials const & wav
    , file::credentials const & fc)
{
    json elem;
    init_attachment(elem, fc);

    if (fc.mime == mime::mime_enum::audio__wav
            && wav.num_channels > 0 && wav.num_channels <= 2) {
        elem[AU_WAV_KEY][AU_DURATION_KEY] = wav.duration;
        elem[AU_WAV_KEY][AU_NUM_CHAN_KEY] = wav.num_channels;

        elem[AU_WAV_KEY][AU_MIN_FRAME_KEY][0] = wav.min_frame.first;
        elem[AU_WAV_KEY][AU_MAX_FRAME_KEY][0] = wav.max_frame.first;

        if (wav.num_channels == 2) {
            elem[AU_WAV_KEY][AU_MIN_FRAME_KEY][1] = wav.min_frame.second;
            elem[AU_WAV_KEY][AU_MAX_FRAME_KEY][1] = wav.max_frame.second;
        }

        for (std::size_t i = 0, count = wav.data.size(); i < count; i++) {
            elem[AU_WAV_KEY][AU_SPECTRUM][i][0] = wav.data[i].first;

            if (wav.num_channels == 2)
                elem[AU_WAV_KEY][AU_SPECTRUM][i][1] = wav.data[i].second;
        }
    }

    _d.push_back(std::move(elem));
}

void content::attach (file::credentials const & fc)
{
    json elem;
    init_attachment(elem, fc);
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
