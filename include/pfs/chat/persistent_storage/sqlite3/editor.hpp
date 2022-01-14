////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"
#include "pfs/chat/basic_editor.hpp"
#include "pfs/chat/emoji_db.hpp"
#include "pfs/chat/error.hpp"
#include "pfs/chat/message.hpp"

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

class conversation;

class editor final: public basic_editor<editor>
{
    friend class basic_editor<editor>;
    friend class conversation;

    using base_class = basic_editor<editor>;

private:
    message::outgoing_credentials _m;
    std::string _content;

protected:
    auto ready () const noexcept -> bool
    {
        return _m.id != message::message_id{};
    }

    auto add_text_impl (std::string const & text) -> bool;
    auto add_emoji_impl (std::string const & shortcode, error * perr) -> bool;
    auto attach_impl (pfs::filesystem::path const & path, error * perr) -> bool;
    auto attach_audio_impl (message::resource_id rc, error * perr) -> bool;
    auto attach_video_impl (message::resource_id rc, error * perr) -> bool;
    auto save_impl (error * perr) -> bool;

private:
    editor () = default;
    editor (editor const & other) = delete;
    editor & operator = (editor const & other) = delete;
    editor & operator = (editor && other) = delete;

    editor (message::outgoing_credentials && m);

public:
    editor (editor && other) = default;
    ~editor () = default;
};

}}} // namespace chat::persistent_storage::sqlite3
