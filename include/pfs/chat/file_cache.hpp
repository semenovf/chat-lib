////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.07.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
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
     * Stores the outgoing file credentials.
     *
     * @return Stored file credentials.
     */
    CHAT__EXPORT file::file_credentials store_outgoing_file (std::string const & uri
        , std::string const & display_name
        , std::int64_t size
        , pfs::utc_time modtime);

    /**
     * Stores the outgoing file credentials.
     *
     * @return Stored file credentials.
     */
    CHAT__EXPORT file::file_credentials store_outgoing_file (
        pfs::filesystem::path const & path);

    /**
     * Stores the incoming file credentials.
     */
    CHAT__EXPORT void store_incoming_file (contact::id author_id
        , file::id file_id
        , pfs::filesystem::path const & path);

    /**
     * Loads outgoing file credentials by specified unique identifier @a file_id.
     *
     * @return Not @c nullopt if file credentials found and loaded successfully
     *         or @c nullopt otherwise.
     */
    CHAT__EXPORT file::optional_file_credentials outgoing_file (file::id file_id) const;

    /**
     * Loads incoming file credentials by specified unique identifier @a file_id.
     *
     * @param file_id Unique incoming file identifier.
     * @param author_id Pointer to contact identifier to store the file author.
     *
     * @return Not @c nullopt if file credentials found and loaded successfully
     *         or @c nullopt otherwise.
     */
    CHAT__EXPORT file::optional_file_credentials incoming_file (file::id file_id
        , contact::id * author_id_ptr = nullptr) const;

    /**
     * Removes broken outgoing and incoming file credentials (when there is no
     * file in file system)
     *
     * @throws error @c errc::storage_error on storage error.
     */
    CHAT__EXPORT void remove_broken ();

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
    /**
     * Removes outgoing file credentials from cache by specified unique identifier
     * @a file_id.
     */
    void remove_outgoing_file (file::id file_id);

    /**
     * Removes incoming file credentials from cache by specified unique identifier
     * @a file_id.
     */
    void remove_incoming_file (file::id file_id);

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
