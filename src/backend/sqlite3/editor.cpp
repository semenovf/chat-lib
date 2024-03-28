////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.04 Initial version.
//      2022.02.17 Refactored totally.
////////////////////////////////////////////////////////////////////////////////
#include "content_traits.hpp"
#include "pfs/assert.hpp"
#include "pfs/numeric_cast.hpp"
#include "pfs/chat/editor.hpp"
#include "pfs/chat/backend/sqlite3/conversation.hpp"
#include "pfs/ionik/audio/wav_explorer.hpp"

#if _MSC_VER
#   include <io.h>
#   include <fcntl.h>
#   include <share.h>
#endif

#if __GNUC__
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <string.h>
#endif

namespace chat {

namespace fs = pfs::filesystem;

namespace backend {
namespace sqlite3 {

editor::rep_type
editor::make (conversation::rep_type * convers, message::id message_id, bool modification)
{
    rep_type rep;
    rep.convers    = convers;
    rep.message_id = message_id;
    rep.mod = modification;
    return rep;
}

editor::rep_type
editor::make (conversation::rep_type * convers
    , message::id message_id
    , message::content && content
    , bool modification)
{
    rep_type rep;
    rep.convers    = convers;
    rep.message_id = message_id;
    rep.content    = std::move(content);
    rep.mod = modification;

    return rep;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::editor;

template <>
editor<BACKEND>::editor ()
{}

template <>
editor<BACKEND>::editor (editor && other)
    : _rep(std::move(other._rep))
    , cache_outgoing_local_file(std::move(other.cache_outgoing_local_file))
    , cache_outgoing_custom_file(std::move(other.cache_outgoing_custom_file))
{
    other.cache_outgoing_local_file = nullptr;
    other.cache_outgoing_custom_file = nullptr;
}

template <>
editor<BACKEND>::editor (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
editor<BACKEND>::~editor ()
{}

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
editor<BACKEND>::add_audio_wav (pfs::filesystem::path const & path)
{
    auto attachment_index = pfs::numeric_cast<std::int16_t>(_rep.content.count());
    auto fc = cache_outgoing_local_file(_rep.message_id, attachment_index, path);

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

            _rep.content.add_audio_wav(wav, fc);
            return;
        }
    }

    _rep.content.attach(fc);
}

template <>
void
editor<BACKEND>::add_live_video_started (std::string const & sdp_desc)
{
    message::live_video_credentials lvc;
    lvc.description = sdp_desc;
    _rep.content.add_live_video(lvc);
}

template <>
void
editor<BACKEND>::add_live_video_stopped ()
{
    message::live_video_credentials lvc;
    lvc.description = "-";
    _rep.content.add_live_video(lvc);
}

template <>
void
editor<BACKEND>::attach (pfs::filesystem::path const & path)
{
    auto attachment_index = pfs::numeric_cast<std::int16_t>(_rep.content.count());
    auto fc = cache_outgoing_local_file(_rep.message_id, attachment_index, path);
    _rep.content.attach(fc);
}

template <>
void
editor<BACKEND>::attach (std::string const & uri, std::string const & display_name
        , std::int64_t size, pfs::utc_time modtime)
{
    auto attachment_index = pfs::numeric_cast<std::int16_t>(_rep.content.count());
    auto fc = cache_outgoing_custom_file(_rep.message_id, attachment_index
        , uri, display_name, size, modtime);
    _rep.content.attach(fc);
}

template <>
void
editor<BACKEND>::clear ()
{
    _rep.content.clear();
}

static std::string const INSERT_MESSAGE {
    "INSERT INTO \"{}\" (message_id, author_id, creation_time, modification_time, content)"
    " VALUES (:message_id, :author_id, :creation_time, :modification_time, :content)"
};

static std::string const DELETE_MESSAGE {
    "DELETE FROM \"{}\" WHERE message_id = :message_id"
};

static std::string const MODIFY_CONTENT {
    "UPDATE OR IGNORE \"{}\" SET content = :content"
    ", modification_time = :modification_time"
    " WHERE message_id = :message_id"
};

template <>
void
editor<BACKEND>::save ()
{
    if (_rep.content.empty()) {
        // Remove message if already initialized.
        if (_rep.message_id != message::id{}) {
            auto stmt = _rep.convers->dbh->prepare(fmt::format(DELETE_MESSAGE
                , _rep.convers->table_name));
            stmt.bind(":message_id", _rep.message_id);
            stmt.exec();
            (*_rep.convers->invalidate_cache)(_rep.convers);
        }
    } else {
        if (!_rep.mod) {
            // Create/save new message
            auto creation_time = pfs::current_utc_time_point();

            auto stmt = _rep.convers->dbh->prepare(fmt::format(INSERT_MESSAGE
                , _rep.convers->table_name));

            stmt.bind(":message_id"       , _rep.message_id);
            stmt.bind(":author_id"        , _rep.convers->author_id);
            stmt.bind(":creation_time"    , creation_time);
            stmt.bind(":modification_time", creation_time);
            stmt.bind(":content"          , _rep.content);

            stmt.exec();

            PFS__ASSERT(stmt.rows_affected() > 0, "Non-unique ID generated for message");
        } else {
            // Modify content
            auto stmt = _rep.convers->dbh->prepare(fmt::format(MODIFY_CONTENT
                , _rep.convers->table_name));

            auto now = pfs::current_utc_time_point();

            stmt.bind(":content", _rep.content);
            stmt.bind(":message_id", _rep.message_id);
            stmt.bind(":modification_time", now);
            stmt.exec();
        }

        (*_rep.convers->invalidate_cache)(_rep.convers);
    }
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
