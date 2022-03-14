////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.04 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "message.hpp"
#include <pfs/filesystem.hpp>
#include <string>

namespace chat {

template <typename Backend>
class conversation;

template <typename Backend>
class editor final
{
    template <typename B>
    friend class conversation;

    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    editor () = default;
    editor (rep_type && rep);
    editor (editor const & other) = delete;
    editor & operator = (editor const & other) = delete;
    editor & operator = (editor && other) = delete;

public:
    editor (editor && other) = default;
    ~editor () = default;

public:
    /**
     * Checks if editor ready for use.
     */
    operator bool () const noexcept;

    /**
     * Add text to message content.
     */
    void add_text (std::string const & text);

    /**
     * Add HTML to message content.
     */
    void add_html (std::string const & text);

    /**
     * Add emoji to message content.
     */
    void add_emoji (std::string const & shortcode);

//     bool add_audio (message::resource_id rc)
//     bool add_video (message::resource_id rc)

    /**
     * Add attachment to message content.
     *
     * @throw chat::error @c errc::attachment_failure.
     */
    void attach (pfs::filesystem::path const & path);

    /**
     * Save message content.
     *
     * @throw debby::error.
     */
    void save ();

    /**
     * Get message content.
     */
    message::content const & content () const noexcept;

    /**
     * Get message ID.
     */
    message::message_id message_id () const noexcept;

public:
    template <typename ...Args>
    static editor make (Args &&... args)
    {
        return editor{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
