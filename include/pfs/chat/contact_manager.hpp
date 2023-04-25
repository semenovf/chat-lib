////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.28 Initial version.
//      2022.02.16 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "contact_list.hpp"
#include "error.hpp"
#include "flags.hpp"
#include "member_difference.hpp"
#include "backend/in_memory/contact_list.hpp"
#include "pfs/memory.hpp"
#include "pfs/optional.hpp"
#include "pfs/time_point.hpp"
#include <functional>
#include <memory>
#include <tuple>
#include <utility>

namespace chat {

enum class contact_novelty
{
      added   = 0
    , updated = 1
};

enum class contact_sort_flag: int
{
      by_nothing = 0
    , no_order = 0
    , by_alias = 1 << 0

    , ascending_order  = 1 << 8
    , descending_order = 1 << 9
};

template <typename Backend>
class contact_manager final
{
    using rep_type = typename Backend::rep_type;

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
         * @return @c true if new member added or @c false if member alread
         *         exists.
         *
         * @throw chat::error{errc::storage_error} on storage error.
         */
        CHAT__EXPORT bool add_member_unchecked (contact::id member_id);

        /**
         * Adds member specified by @a member_id to the group specified
         * by @a group_id.
         *
         * @return @c true if new member added or @c false if member already
         *         exists.
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
    rep_type _rep;

private:
    contact_manager () = delete;
    CHAT__EXPORT contact_manager (rep_type && rep);
    contact_manager (contact_manager const & other) = delete;
    contact_manager & operator = (contact_manager const & other) = delete;
    contact_manager & operator = (contact_manager && other) = delete;

public:
    contact_manager (contact_manager && other) = default;
    ~contact_manager () = default;

private:
    /**
     * Adds contact.
     *
     * @return @c true if contact successfully added or @c false if contact
     *         already exists with @c contact_id.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    bool add (contact::contact const & c);

    /**
     * Update contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    bool update (contact::contact const & c);

public:
    /**
     * Checks if contact manager opened/initialized successfully.
     */
    CHAT__EXPORT operator bool () const noexcept;

    CHAT__EXPORT contact::person my_contact () const;

    /**
     * Changes @a alias for my contact.
     */
    CHAT__EXPORT void change_my_alias (std::string const & alias);

    /**
     * Changes @a avatar for my contact.
     */
    CHAT__EXPORT void change_my_avatar (std::string const & avatar);

    /**
     * Changes description for my contact.
     */
    CHAT__EXPORT void change_my_desc (std::string const & desc);

    /**
     * Total count of contacts.
     */
    CHAT__EXPORT std::size_t count () const;

    /**
     * Count of contacts with specified type.
     */
    CHAT__EXPORT std::size_t count (conversation_enum type) const;

    /**
     * Count of person contacts.
     */
    std::size_t person_count () const
    {
        return count(conversation_enum::person);
    }

    /**
     * Count of group contacts.
     */
    std::size_t groups_count () const
    {
        return count(conversation_enum::group);
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
    CHAT__EXPORT bool add (contact::person const & p);

    /**
     * Add group contact. @a creator_id also added to group.
     *
     * @return @c true if group contact successfully added or @c false if group
     *         contact already exists with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     *
     */
    CHAT__EXPORT bool add (contact::group const & g);

    /**
     * Updates person contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT bool update (contact::person const & p);

    /**
     * Updates group contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT bool update (contact::group const & g);

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
     * Wipes (erase all contacts, groups and channels) contact database.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT void wipe ();

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
     * Execute transaction (batch execution). Useful for storages that support
     * tranactions
     */
    CHAT__EXPORT bool transaction (std::function<bool()> op) noexcept;

    template <typename U = backend::in_memory::contact_list>
    CHAT__EXPORT contact_list<U> contacts (std::function<bool(contact::contact const &)> f
        = [] (contact::contact const &) { return true; }) const;

public:
    template <typename ...Args>
    static contact_manager make (Args &&... args)
    {
        return contact_manager{Backend::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<contact_manager> make_unique (Args &&... args)
    {
        auto ptr = new contact_manager {Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<contact_manager>(ptr);
    }
};

} // namespace chat
