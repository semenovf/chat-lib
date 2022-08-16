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
#include "pfs/chat/editor.hpp"
#include "pfs/chat/backend/sqlite3/conversation.hpp"

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
editor::make (conversation::rep_type * convers, message::id message_id)
{
    rep_type rep;
    rep.convers    = convers;
    rep.message_id = message_id;
    return rep;
}

editor::rep_type
editor::make (conversation::rep_type * convers
    , message::id message_id
    , message::content && content)
{
    rep_type rep;
    rep.convers    = convers;
    rep.message_id = message_id;
    rep.content    = std::move(content);

    return rep;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::editor;

template <>
editor<BACKEND>::editor (rep_type && rep)
    : _rep(std::move(rep))
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
editor<BACKEND>::attach (file::file_credentials const & fc)
{
    _rep.content.attach(fc);
}

template <>
void
editor<BACKEND>::clear ()
{
    _rep.content.clear();
}

static std::string const INSERT_MESSAGE {
    "INSERT INTO `{}` (`message_id`, `author_id`, `creation_time`, `modification_time`, `content`)"
    " VALUES (:message_id, :author_id, :creation_time, :modification_time, :content)"
};

static std::string const DELETE_MESSAGE {
    "DELETE FROM `{}` WHERE `message_id` = :message_id"
};

static std::string const MODIFY_CONTENT {
    "UPDATE OR IGNORE `{}` SET `content` = :content"
    ", `modification_time` = :modification_time"
    " WHERE `message_id` = :message_id"
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
            PFS__ASSERT(!!stmt, "");
            stmt.bind(":message_id", _rep.message_id);
            stmt.exec();
            (*_rep.convers->invalidate_cache)(_rep.convers);
        }
    } else {
        bool modification = _rep.message_id != message::id{};

        if (!modification) {
            // Create/save new message
            _rep.message_id = message::id_generator{}.next();
            auto creation_time = pfs::current_utc_time_point();

            auto stmt = _rep.convers->dbh->prepare(fmt::format(INSERT_MESSAGE
                , _rep.convers->table_name));

            PFS__ASSERT(!!stmt, "");

            stmt.bind(":message_id", _rep.message_id);
            stmt.bind(":author_id", _rep.convers->me);
            stmt.bind(":creation_time", creation_time);
            stmt.bind(":modification_time", creation_time);
            stmt.bind(":content", _rep.content);

            stmt.exec();

            PFS__ASSERT(stmt.rows_affected() > 0, "Non-unique ID generated for message");
        } else {
            // Modify content
            auto stmt = _rep.convers->dbh->prepare(fmt::format(MODIFY_CONTENT
                , _rep.convers->table_name));

            PFS__ASSERT(!!stmt, "");

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
