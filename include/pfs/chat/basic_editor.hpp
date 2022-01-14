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

    auto add_text (std::string const & text) -> bool
    {
        return static_cast<Impl *>(this)->add_text_impl(text);
    }

    auto add_emoji (std::string const & shortcode, error * perr = nullptr) -> bool
    {
        return static_cast<Impl *>(this)->add_emoji_impl(shortcode, perr);
    }

    auto attach (pfs::filesystem::path const & path, error * perr = nullptr) -> bool
    {
        return static_cast<Impl *>(this)->attach_impl(path, perr);
    }

    auto attach_audio (message::resource_id rc, error * perr = nullptr) -> bool
    {
        return static_cast<Impl *>(this)->attach_audio_impl(rc, perr);
    }

    auto attach_video (message::resource_id rc, error * perr = nullptr) -> bool
    {
        return static_cast<Impl *>(this)->attach_video_impl(rc, perr);
    }

    auto save (error * perr = nullptr) -> bool
    {
        return static_cast<Impl *>(this)->save_impl(perr);
    }
};

} // namespace chat
