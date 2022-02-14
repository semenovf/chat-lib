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
    database_handle_t   _dbh;
    std::string         _table_name;
    message::message_id _message_id;
    message::content    _content;

    //message::outgoing_credentials _m;

protected:
    auto ready () const noexcept -> bool
    {
        return _message_id != message::message_id{};
    }

    void add_text_impl (std::string const & text);
    void add_html_impl (std::string const & text);
    void add_emoji_impl (std::string const & shortcode);
    bool attach_impl (pfs::filesystem::path const & path, error * perr);
    bool save_impl (error * perr);

    message::content const & content_impl () const noexcept;
    message::message_id message_id_impl () const noexcept;

private:
    editor () = default;
    editor (editor const & other) = delete;
    editor & operator = (editor const & other) = delete;
    editor & operator = (editor && other) = delete;

    editor (message::message_id message_id
        , database_handle_t dbh
        , std::string const & table_name);

    editor (message::message_id message_id
        , message::content && content
        , database_handle_t dbh
        , std::string const & table_name);

public:
    editor (editor && other) = default;
    ~editor () = default;
};

}}} // namespace chat::persistent_storage::sqlite3
