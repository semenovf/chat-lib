////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.11 Initial version.
//      2024.12.01 Started V2.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include <pfs/optional.hpp>
#include <pfs/time_point.hpp>
#include <functional>
#include <memory>

namespace chat {

enum class contact_activity
{
      offline = 1
    , online  = 2
};

struct activity_entry
{
    pfs::optional<pfs::utc_time> offline_utc_time;
    pfs::optional<pfs::utc_time> online_utc_time;
};

template <typename Storage>
class activity_manager final
{
    using rep = typename Storage::activity_manager;

private:
    std::unique_ptr<rep> _d;

public:
    CHAT__EXPORT activity_manager (activity_manager && other) noexcept;
    CHAT__EXPORT activity_manager & operator = (activity_manager && other) noexcept;
    CHAT__EXPORT ~activity_manager ();

    // For internal use only
    CHAT__EXPORT activity_manager (rep * d) noexcept;

    activity_manager (activity_manager const & other) = delete;
    activity_manager & operator = (activity_manager const & other) = delete;

public:
    /**
     * Checks if activity manager opened/initialized successfully.
     */
    CHAT__EXPORT operator bool () const noexcept;


    /**
     * Clears all activities.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT void clear ();

    /**
     * Logs contact activity.
     *
     * @param id Contact identifier.
     * @param ca Contact activity value.
     * @param time Time when contact activity occurred.
     * @param brief_only If @c true only brief information (last specified
     *        activity time) will be updated, otherwise a new record will be
     *        added to the log of all contact activities.
     */
    CHAT__EXPORT void log_activity (contact::id id, contact_activity ca
        , pfs::utc_time const & time, bool brief_only);

    /**
     * Get last activity time specified by @a ca.
     */
    CHAT__EXPORT pfs::optional<pfs::utc_time> last_activity (contact::id id
        , contact_activity ca);

    /**
     * Get last activity brief
     */
    CHAT__EXPORT activity_entry last_activity (contact::id id);

    /**
     * Clear activities for specfied contact.
     */
    CHAT__EXPORT void clear_activities (contact::id id);

    /**
     * Clear activities for all contacts.
     */
    CHAT__EXPORT void clear_activities ();

    /**
     * Iterate all activity log entries for specified contact.
     */
    CHAT__EXPORT void for_each_activity (contact::id id
        , std::function<void(contact_activity ca, pfs::utc_time const &)> f);

    /**
     * Iterate all activity log entries for all contacts.
     */
    CHAT__EXPORT void for_each_activity (
          std::function<void(contact::id, contact_activity ca, pfs::utc_time const &)> f);

    /**
     * Iterate all activity brief log entries.
     */
    CHAT__EXPORT void for_each_activity_brief (
          std::function<void(contact::id, pfs::optional<pfs::utc_time> const &/*online*/
            , pfs::optional<pfs::utc_time> const &/*offline*/)> f);

public:
    template <typename ...Args>
    static activity_manager make (Args &&... args)
    {
        return activity_manager{Storage::make_activity_manager(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<activity_manager> make_unique (Args &&... args)
    {
        return std::make_unique<activity_manager>(Storage::make_activity_manager(std::forward<Args>(args)...));
    }
};

} // namespace chat
