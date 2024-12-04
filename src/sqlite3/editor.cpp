////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.04 Initial version.
//      2022.02.17 Refactored totally.
//      2024.12.01 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "editor_impl.hpp"
#include "chat/editor.hpp"
#include "chat/error.hpp"
#include "chat/sqlite3.hpp"
#include <pfs/numeric_cast.hpp>
#include <pfs/filesystem.hpp>
#include <pfs/debby/relational_database.hpp>
#include <pfs/ionik/audio/wav_explorer.hpp>

CHAT__NAMESPACE_BEGIN

namespace fs = pfs::filesystem;

using relational_database_t = debby::relational_database<debby::backend_enum::sqlite3>;
using editor_t = editor<storage::sqlite3>;

template <>
editor_t::editor (editor && other) noexcept
    : _d(std::move(other._d))
    , cache_outgoing_local_file(std::move(other.cache_outgoing_local_file))
    , cache_outgoing_custom_file(std::move(other.cache_outgoing_custom_file))
{
    other.cache_outgoing_local_file = nullptr;
    other.cache_outgoing_custom_file = nullptr;
}

template <>
editor_t & editor_t::operator = (editor && other) noexcept
{
    _d = std::move(other._d);
    cache_outgoing_local_file = std::move(other.cache_outgoing_local_file);
    cache_outgoing_custom_file = std::move(other.cache_outgoing_custom_file);
    other.cache_outgoing_local_file = nullptr;
    other.cache_outgoing_custom_file = nullptr;
    return *this;
}

template <> editor_t::editor (rep * d) noexcept
    : _d(d)
{}

template <> editor_t::~editor () = default;

template <>
void editor_t::add_text (std::string const & text)
{
    _d->content.add_text(text);
}

template <>
void editor_t::add_html (std::string const & text)
{
    _d->content.add_html(text);
}

template <>
void editor_t::add_audio_wav (fs::path const & path)
{
    auto attachment_index = pfs::numeric_cast<std::int16_t>(_d->content.count());
    auto fc = cache_outgoing_local_file(_d->message_id, attachment_index, path);

    ////////////////////////////////////
    ionik::audio::wav_explorer explorer {path};
    ionik::audio::wav_spectrum_builder spectrum_builder {explorer};

    std::size_t chunk_count = 40;
    auto res = spectrum_builder(chunk_count);

    if (res) {
        if (res->info.num_channels > 0 && res->info.num_channels <= 2) {
            message::audio_wav_credentials wav;
            wav.num_channels = pfs::numeric_cast<std::uint8_t>(res->info.num_channels);
            wav.duration = res->info.duration / 1000; // Milliseconds
            wav.min_frame = res->min_frame;
            wav.max_frame = res->max_frame;
            wav.data = res->data;

            _d->content.add_audio_wav(wav, fc);
            return;
        }
    }

    _d->content.attach(fc);
}

template <>
void editor_t::add_live_video_started (std::string const & sdp_desc)
{
    message::live_video_credentials lvc;
    lvc.description = sdp_desc;
    _d->content.add_live_video(lvc);
}

template <>
void editor_t::add_live_video_stopped ()
{
    message::live_video_credentials lvc;
    lvc.description = "-";
    _d->content.add_live_video(lvc);
}

template <>
void editor_t::attach (fs::path const & path)
{
    auto attachment_index = pfs::numeric_cast<std::int16_t>(_d->content.count());
    auto fc = cache_outgoing_local_file(_d->message_id, attachment_index, path);
    _d->content.attach(fc);
}

template <>
void editor_t::attach (std::string const & uri, std::string const & display_name
    , std::int64_t size, pfs::utc_time modtime)
{
    auto attachment_index = pfs::numeric_cast<std::int16_t>(_d->content.count());
    auto fc = cache_outgoing_custom_file(_d->message_id, attachment_index
        , uri, display_name, size, modtime);
    _d->content.attach(fc);
}

template <>
void editor_t::clear ()
{
    _d->content.clear();
}

template <>
void editor_t::save ()
{
    static std::string const INSERT_MESSAGE {
        "INSERT INTO \"{}\" (message_id, author_id, creation_time, modification_time, content)"
        " VALUES (:message_id, :author_id, :creation_time, :modification_time, :content)"
    };

    static std::string const DELETE_MESSAGE {
        "DELETE FROM \"{}\" WHERE message_id = :message_id"
    };

    static std::string const MODIFY_CONTENT {
        "UPDATE OR IGNORE \"{}\" SET content = :content"
        ", modification_time = :modification_time WHERE message_id = :message_id"
    };

    debby::error err;

    if (_d->content.empty()) {
        // Remove message if already initialized.
        if (_d->message_id != message::id{}) {
            auto stmt = _d->holder->pdb->prepare_cached(fmt::format(DELETE_MESSAGE, _d->holder->table_name), & err);

            if (!err) {
                stmt.bind(":message_id", _d->message_id, & err);

                if (!err) {
                    stmt.exec(& err);
                    _d->holder->invalidate_cache();
                }
            }
       }
    } else {
        if (_d->mode == editor_mode::create) {
            // Create/save new message
            auto creation_time = pfs::current_utc_time_point();

            auto stmt = _d->holder->pdb->prepare_cached(fmt::format(INSERT_MESSAGE
                , _d->holder->table_name), & err);

            stmt.bind(":message_id", _d->message_id, & err)
                && stmt.bind(":author_id", _d->holder->author_id, & err)
                && stmt.bind(":creation_time"    , creation_time, & err)
                && stmt.bind(":modification_time", creation_time, & err)
                && stmt.bind(":content", to_string(_d->content), & err);

            if (!err)
               stmt.exec(& err);
        } else {
            // Modify content
            auto stmt = _d->holder->pdb->prepare_cached(fmt::format(MODIFY_CONTENT, _d->holder->table_name), & err);

            if (!err) {
                auto now = pfs::current_utc_time_point();
                stmt.bind(":content", to_string(_d->content), & err)
                    && stmt.bind(":message_id", _d->message_id, & err)
                    && stmt.bind(":modification_time", now, & err);

                if (!err) {
                    stmt.exec(& err);
                    _d->holder->invalidate_cache();
                }
            }
        }
    }

    if (err) {
        throw error {errc::storage_error, tr::_("save message failure"), err.what()};
    }
}

template <>
message::content const & editor_t::content () const noexcept
{
    return _d->content;
}

template <>
message::id editor_t::message_id () const noexcept
{
    return _d->message_id;
}

CHAT__NAMESPACE_END
