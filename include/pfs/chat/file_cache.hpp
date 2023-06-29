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
#include <vector>

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
    CHAT__EXPORT file::credentials cache_outgoing_file (contact::id author_id
        , contact::id conversation_id, message::id message_id
        , std::int16_t attachment_index,  pfs::filesystem::path const & path);

    /**
     * Stores the outgoing file credentials.
     *
     * @return Stored file credentials.
     */
    CHAT__EXPORT file::credentials cache_outgoing_file (contact::id author_id
        , contact::id conversation_id
        , message::id message_id
        , std::int16_t attachment_index
        , std::string const & uri
        , std::string const & display_name
        , std::int64_t size
        , pfs::utc_time modtime);

    /**
     * Reserve incoming file credentials in cache.
     */
    CHAT__EXPORT void reserve_incoming_file (file::id file_id
        , contact::id author_id
        , contact::id conversation_id
        , message::id message_id
        , std::int16_t attachment_index
        , std::string const & name
        , std::size_t size
        , mime_enum mime);

    /**
     * Commits (stores absolute path) the incoming file.
     */
    CHAT__EXPORT void commit_incoming_file (file::id file_id
        , pfs::filesystem::path const & path);

    /**
     * Loads outgoing file credentials by specified unique identifier @a file_id.
     *
     * @return Not @c nullopt if file credentials found and loaded successfully
     *         or @c nullopt otherwise.
     */
    CHAT__EXPORT file::optional_credentials outgoing_file (file::id file_id) const;

    /**
     * Loads incoming file credentials by specified unique identifier @a file_id.
     *
     * @param file_id Unique incoming file identifier.
     * @param author_id Pointer to contact identifier to store the file author.
     *
     * @return Not @c nullopt if file credentials found and loaded successfully
     *         or @c nullopt otherwise.
     */
    CHAT__EXPORT file::optional_credentials incoming_file (file::id file_id) const;

    /**
     * Total list of incoming files (attachments) from specified opponent.
     */
    CHAT__EXPORT std::vector<file::credentials> incoming_files (contact::id conversation_id) const;

    /**
     * Total list of outgoing files (attachments) for specified opponent.
     */
    CHAT__EXPORT std::vector<file::credentials> outgoing_files (contact::id conversation_id) const;

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
