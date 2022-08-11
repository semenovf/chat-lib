////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.07.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "file.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/universal_id.hpp"

namespace chat {

template <typename Backend>
class file_cache
{
    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    file_cache () = delete;
    CHAT__EXPORT file_cache (rep_type && rep);
    file_cache (file_cache const & other) = delete;
    file_cache & operator = (file_cache const & other) = delete;
    file_cache & operator = (file_cache && other) = delete;

public:
    file_cache (file_cache && other) = default;
    ~file_cache () = default;

public:
    /**
     * Checks if file cache store opened/initialized successfully.
     */
    CHAT__EXPORT operator bool () const noexcept;

    /**
     * Stores the credentials if they don't exist in the cache, or loads them
     * otherwise.
     */
    file::file_credentials ensure (pfs::filesystem::path const & path);

    /**
     * Remove broken credentials (when there is no file in file system)
     *
     * @return Number of broken credentials.
     *
     * @throws error @c errc::storage_error on storage error.
     */
    std::size_t remove_broken ();

    /**
     * Clear all file credentials.
     *
     * @throw chat::error @c errc::storage_error on storage error.
     */
    CHAT__EXPORT void clear () noexcept;

    /**
     * Wipes (drop) file cache.
     *
     * @throw chat::error @c errc::storage_error on storage error.
     */
    CHAT__EXPORT void wipe () noexcept;

private:
    file::file_credentials load (file::id fileid);
    file::file_credentials load (pfs::filesystem::path const & abspath);

public:
    /**
     * Makes/initializes file cache.
     */
    template <typename ...Args>
    static file_cache make (Args &&... args)
    {
        return file_cache{Backend::make(std::forward<Args>(args)...)};
    }

    /**
     * Makes/initializes file cache.
     */
    template <typename ...Args>
    static std::unique_ptr<file_cache> make_unique (Args &&... args)
    {
        auto ptr = new file_cache{Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<file_cache>(ptr);
    }
};

} // namespace chat
