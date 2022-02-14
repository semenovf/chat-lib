////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "message.hpp"
#include <pfs/filesystem.hpp>
#include <string>

namespace chat {

template <typename Impl>
class basic_editor
{
public:
    /**
     * Checks if editor opened successfully.
     */
    operator bool () const noexcept
    {
        return static_cast<Impl const *>(this)->ready();
    }

    void add_text (std::string const & text)
    {
        return static_cast<Impl *>(this)->add_text_impl(text);
    }

    void add_html (std::string const & text)
    {
        return static_cast<Impl *>(this)->add_html_impl(text);
    }

    void add_emoji (std::string const & shortcode)
    {
        return static_cast<Impl *>(this)->add_emoji_impl(shortcode);
    }

//     auto add_audio (message::resource_id rc, error * perr = nullptr) -> bool
//     {
//         return static_cast<Impl *>(this)->attach_audio_impl(rc, perr);
//     }
//
//     auto add_video (message::resource_id rc, error * perr = nullptr) -> bool
//     {
//         return static_cast<Impl *>(this)->attach_video_impl(rc, perr);
//     }

    bool attach (pfs::filesystem::path const & path, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->attach_impl(path, perr);
    }

    bool save (error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->save_impl(perr);
    }

    message::content const & content () const noexcept
    {
        return static_cast<Impl const *>(this)->content_impl();
    }

    message::message_id message_id () const noexcept
    {
        return static_cast<Impl const *>(this)->message_id_impl();
    }
};

} // namespace chat
