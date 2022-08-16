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
#include "exports.hpp"
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
    CHAT__EXPORT editor (rep_type && rep);
    editor (editor const & other) = delete;
    editor & operator = (editor const & other) = delete;

public:
    editor (editor && other) = default;
    editor & operator = (editor && other) = delete;
    ~editor () = default;

public:
    /**
     * Add text to message content.
     */
    CHAT__EXPORT void add_text (std::string const & text);

    /**
     * Add HTML to message content.
     */
    CHAT__EXPORT void add_html (std::string const & text);

    /**
     * Add audio to message content.
     *
     * TODO Implement
     */
    //CHAT__EXPORT void add_audio (pfs::filesystem::path const & path);

    /**
     * Add video to message content.
     *
     * TODO Implement
     */
    //CHAT__EXPORT void add_video (pfs::filesystem::path const & path);

    /**
     * Add attachment to message content.
     *
     * @throw chat::error @c errc::attachment_failure.
     */
    CHAT__EXPORT void attach (file::file_credentials const & fc);

    /**
     * Clear message content.
     */
    CHAT__EXPORT void clear ();

    /**
     * Save content.
     */
    CHAT__EXPORT void save ();

    /**
     * Get message content.
     */
    CHAT__EXPORT message::content const & content () const noexcept;

    /**
     * Get message ID.
     */
    CHAT__EXPORT message::id message_id () const noexcept;

private:
    template <typename ...Args>
    static editor make (Args &&... args)
    {
        return editor{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
