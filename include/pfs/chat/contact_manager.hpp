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
#include "error.hpp"
#include "member_difference.hpp"
#include "pfs/memory.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace chat {

enum class contact_novelty
{
      added
    , updated
};

template <typename Backend>
class contact_manager final
{
    using rep_type = typename Backend::rep_type;

public:
    using contact_list_type = typename Backend::contact_list_type;

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

    bool update (contact::contact const & c);

public:
    contact_manager (contact_manager && other) = default;
    ~contact_manager () = default;

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
    CHAT__EXPORT std::size_t count (contact::type_enum type) const;

    /**
     * Count of person contacts.
     */
    std::size_t person_count () const
    {
        return count(contact::type_enum::person);
    }

    /**
     * Count of group contacts.
     */
    std::size_t groups_count () const
    {
        return count(contact::type_enum::group);
    }

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
    bool update (contact::person const & p)
    {
        contact::contact c {
              p.contact_id
            , p.alias
            , p.avatar
            , p.description
            , p.contact_id
            , chat::contact::type_enum::person
        };

        return update(c);
    }

    /**
     * Updates group contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    bool update (contact::group const & g)
    {
        contact::contact c {
              g.contact_id
            , g.alias
            , g.avatar
            , g.description
            , g.creator_id
            , chat::contact::type_enum::group };
        return update(c);
    }

    /**
     * Group reference if @a group_id is identifier of exist group or invalid
     * reference otherwise.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT group_ref gref (contact::id group_id);

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
     * @return Contact specified by @a offset or invalid contact
     *         (see is_valid()) if contact not found by @a offset.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT contact::contact get (int offset) const;

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
    CHAT__EXPORT void for_each (std::function<void(contact::contact const &)> f);

    /**
     * Fetch all contacts and process them by @a f until @f does not
     * return @c false.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    CHAT__EXPORT void for_each_until (std::function<bool(contact::contact const &)> f);

    /**
     * Execute transaction (batch execution). Useful for storages that support
     * tranactions
     */
    CHAT__EXPORT bool transaction (std::function<bool()> op) noexcept;

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
