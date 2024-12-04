////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.28 Initial version.
//      2022.02.16 Refactored to use backend.
//      2024.11.23 Started V2.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "contact_list.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "flags.hpp"
#include "in_memory.hpp"
#include "member_difference.hpp"
#include <pfs/optional.hpp>
#include <pfs/time_point.hpp>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>

namespace chat {

template <typename Storage>
class contact_manager final
{
    using rep = typename Storage::contact_manager;

public:
    class group_const_ref
    {
        friend class contact_manager;

        contact_manager const * _pmanager{nullptr};
        contact::id _id;

    private:
        group_const_ref () : _pmanager(nullptr) {}

        group_const_ref (contact::id id, contact_manager const * pmanager)
            : _pmanager(pmanager)
            , _id(id)
        {}

    public:
        /**
         * Checks if group reference is valid.
         */
        operator bool () const
        {
            return _pmanager != nullptr;
        }

        /**
         * Get members of the specified group.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT std::vector<contact::contact> members () const;

        /**
         * Get member contact identifiers.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT std::vector<contact::id> member_ids () const;

        /**
         * Checks if contact @a member_id is the member of group.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT bool is_member_of (contact::id member_id) const;

        /**
         * Count of members in group.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT std::size_t count () const;
    };

    class group_ref
    {
        friend class contact_manager;

        contact_manager * _pmanager{nullptr};
        contact::id _id;

    private:
        group_ref () : _pmanager(nullptr) {}

        group_ref (contact::id id, contact_manager * pmanager)
            : _pmanager(pmanager)
            , _id(id)
        {}

    public:
        /**
         * Checks if group reference is valid.
         */
        operator bool () const
        {
            return _pmanager != nullptr;
        }

        /**
         * Adds member specified by @a member_id to the group specified by
         * @a group_id without checking @a member_id is person contact.
         *
         * @return @c true if new member added or @c false if member alread exists.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT bool add_member_unchecked (contact::id member_id);

        /**
         * Adds member specified by @a member_id to the group specified
         * by @a group_id.
         *
         * @return @c true if new member added or @c false if member already exists.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         * @throw chat::error{errc::contact_not_found} Contact not found by @a member_id.
         * @throw chat::error{errc::unsuitable_group_member} @a member_id is not a person contact id
         */
        CHAT__EXPORT bool add_member (contact::id member_id);

        /**
         * Removes member from group.
         *
         * @return @c true if member was removed or @c false if member not found.
        *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT bool remove_member (contact::id member_id);

        /**
         * Removes all members from group.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT void remove_all_members ();

        /**
         * Update members in group.
         *
         * @return Group member difference.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT member_difference_result update (std::vector<contact::id> members);

        /**
         * Get members of the specified group.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT std::vector<contact::contact> members () const;

        /**
         * Get member contact identifiers.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT std::vector<contact::id> member_ids () const;

        /**
         * Checks if contact @a member_id is the member of group.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT bool is_member_of (contact::id member_id) const;

        /**
         * Count of members in group.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT std::size_t count () const;
    };

private:
    std::unique_ptr<rep> _d;

private:
    CHAT__EXPORT contact_manager (rep * d) noexcept;

public:
    CHAT__EXPORT contact_manager (contact_manager && other) noexcept;
    CHAT__EXPORT contact_manager & operator = (contact_manager && other) noexcept;
    CHAT__EXPORT ~contact_manager ();

    contact_manager (contact_manager const & other) = delete;
    contact_manager & operator = (contact_manager const & other) = delete;

private:
    /**
     * Adds contact.
     *
     * @return @c true if contact successfully added or @c false if contact
     *         already exists with @c contact_id.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    bool add (contact::contact && c);

    /**
     * Update contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    bool update (contact::contact && c);

public:
    /**
     * Checks if contact manager opened/initialized successfully.
     */
    CHAT__EXPORT operator bool () const noexcept;

    CHAT__EXPORT contact::person my_contact () const;

    /**
     * Changes @a alias for my contact.
     */
    CHAT__EXPORT void change_my_alias (std::string && alias);

    /**
     * Changes @a avatar for my contact.
     */
    CHAT__EXPORT void change_my_avatar (std::string && avatar);

    /**
     * Changes description for my contact.
     */
    CHAT__EXPORT void change_my_desc (std::string && desc);

    /**
     * Total count of contacts.
     */
    CHAT__EXPORT std::size_t count () const;

    /**
     * Count of contacts with specified type.
     */
    CHAT__EXPORT std::size_t count (chat_enum type) const;

    /**
     * Count of person contacts.
     */
    std::size_t person_count () const
    {
        return count(chat_enum::person);
    }

    /**
     * Count of group contacts.
     */
    std::size_t group_count () const
    {
        return count(chat_enum::group);
    }

    /**
     * Get contact by @a id.
     *
     * @return Contact specified by @a id or invalid contact (see is_valid()) if
     *         contact not found by @a id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT contact::contact get (contact::id id) const;

    /**
     * Get contact by @a offset.
     *
     * @return Contact specified by @a id or invalid contact (see is_valid()) if
     *         contact not found by @a id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT contact::contact at (int offset) const;

    /**
     * Add person contact.
     *
     * @return @c true if contact successfully added or @c false if contact
     *         already exists with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     *
     */
    CHAT__EXPORT bool add (contact::person && p);

    /**
     * Add group contact. @a creator_id also added to group.
     *
     * @return @c true if group contact successfully added or @c false if group
     *         contact already exists with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     *
     */
    CHAT__EXPORT bool add (contact::group && g);

    /**
     * Updates person contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT bool update (contact::person && p);

    /**
     * Updates group contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT bool update (contact::group && g);

    /**
     * Conversation group reference if @a group_id is identifier of exist
     * conversation group or invalid reference otherwise.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT group_const_ref gref (contact::id group_id) const;
    CHAT__EXPORT group_ref gref (contact::id group_id);

    /**
     * Count of contacts in specified group. Return @c 0 on error.
     *
     * @throw chat::error{errc::group_not_found} on storage error.
     */
    std::size_t members_count (contact::id group_id) const
    {
        auto group_ref = gref(group_id);

        // Attempt to get members count of non-existent group
        if (!group_ref)
            throw error {errc::group_not_found};

        return group_ref.count();
    }

    /**
     * Removes contact.
     *
     * @details If @a id is a group then group contact and all memberships
     *          will be removed. If @a id is a person contact membership will
     *          be removed in case of group participation.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT void remove (contact::id id);

    /**
     * Clears (erase all contacts, groups and channels) contact database.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT void clear ();

    /**
     * Fetch all contacts and process them by @a f
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT void for_each (std::function<void(contact::contact const &)> f) const;
    CHAT__EXPORT void for_each_movable (std::function<void(contact::contact &&)> f) const;

    /**
     * Fetch all contacts and process them by @a f until @f does not
     * return @c false.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT void for_each_until (std::function<bool(contact::contact const &)> f) const;
    CHAT__EXPORT void for_each_until_movable (std::function<bool(contact::contact &&)> f) const;

    /**
     * Execute transaction (batch execution). Useful for storages that support tranactions
     *
     * @return @c nullopt on success or @c std::string containing an error description otherwise.
     */
    CHAT__EXPORT pfs::optional<std::string> transaction (std::function<pfs::optional<std::string>()> op);

    template <typename ContactList = contact_list<storage::in_memory>>
    CHAT__EXPORT ContactList contacts (std::function<bool(contact::contact const &)> f
        = [] (contact::contact const &) { return true; }) const;

public:
    template <typename ...Args>
    static contact_manager make (Args &&... args)
    {
        return contact_manager{Storage::make_contact_manager(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<contact_manager> make_unique (Args &&... args)
    {
        return std::make_unique<contact_manager>(Storage::make_contact_manager(std::forward<Args>(args)...));
    }
};

} // namespace chat
