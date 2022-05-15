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
#include "pfs/memory.hpp"
#include <functional>
#include <memory>

namespace chat {

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
        contact::contact_id _id;

    private:
        group_ref () : _pmanager(nullptr) {}

        group_ref (contact::contact_id id, contact_manager * pmanager)
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
         * Adds member specified by @a member_id to the group specified by @a group_id
         * without checking @a member_id is person contact.
         *
         * @throw debby::error on storage error.
         */
        bool add_member_unchecked (contact::contact_id member_id);

        /**
         * Adds member specified by @a member_id to the group specified by @a group_id
         * with checking @a creator_id is person contact.
         *
         * @return @c false on error or @c true if contact added was successfully
         *         or it is already a member of the specified group.
         *
         * @throw debby::error on storage error.
         * @throw chat::error{errc::contact_not_found} Contact not found by @a member_id.
         * @throw chat::error{errc::unsuitable_group_member} @a member_id is not a person contact id
         */
        bool add_member (contact::contact_id member_id);

        /**
         * Removes member from group.
         *
         * @throw debby::error on storage error.
         */
        void remove_member (contact::contact_id member_id);

        /**
         * Removes all members from group.
         *
         * @throw debby::error on storage error.
         */
        void remove_all_members ();

        /**
         * Get members of the specified group.
         *
         * @throw debby::error on storage error.
         */
        std::vector<contact::contact> members () const;

        /**
         * Checks if contact @a member_id is the member of group @a group_id.
         *
         * @throw debby::error on storage error.
         */
        bool is_member_of (contact::contact_id member_id) const;

        /**
         * Count of members in group.
         */
        std::size_t count () const;
    };

private:
    rep_type _rep;

private:
    contact_manager () = delete;
    contact_manager (rep_type && rep);
    contact_manager (contact_manager const & other) = delete;
    contact_manager & operator = (contact_manager const & other) = delete;
    contact_manager & operator = (contact_manager && other) = delete;

public:
    contact_manager (contact_manager && other) = default;
    ~contact_manager () = default;

public:
    /**
     * Checks if contact manager opened/initialized successfully.
     */
    operator bool () const noexcept;

    contact::person my_contact () const;

    /**
     * Total count of contacts.
     */
    std::size_t count () const;

    /**
     * Count of contacts with specified type.
     */
    std::size_t count (contact::type_enum type) const;

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
     */
    bool add (contact::person const & p);

    /**
     * Add group contact. @a creator_id also added to group.
     *
     */
    bool add (contact::group const & g, contact::contact_id creator_id);

    /**
     * Updates contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     */
    bool update (contact::contact const & c);

    /**
     * Updates person contact.
     *
     * @return @c true if contact successfully updated or @c false if contact
     *         not found with @c contact_id.
     */
    bool update (contact::person const & p)
    {
        contact::contact c {
              p.id
            , p.id
            , p.alias
            , p.avatar
            , p.description
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
     * @throw debby::error on storage error.
     */
    bool update (contact::group const & g)
    {
        contact::contact c {
              g.id
            , g.creator_id
            , g.alias
            , g.avatar
            , g.description
            , chat::contact::type_enum::group };
        return update(c);
    }

    /**
     * Group reference if @a group_id is identifier of exist group or invalid
     * reference otherwise.
     *
     * @throw debby::error on storage error.
     */
    group_ref gref (contact::contact_id group_id);

    /**
     * Removes contact.
     *
     * @details If @a id is a group then group contact and all memberships
     *          will be removed. If @a id is a person contact membership will
     *          be removed in case of group participation.
     *
     * @throw debby::error on storage error.
     */
    void remove (contact::contact_id id);

    /**
     * Get contact by @a id. On error returns invalid contact.
     *
     * @throw debby::error on storage error.
     */
    contact::contact get (contact::contact_id id) const;

    /**
     * Get contact by @a offset. On error returns invalid contact.
     *
     * @throw debby::error on storage error.
     */
    contact::contact get (int offset) const;

    /**
     * Wipes (erase all contacts, groups and channels) contact database.
     *
     * @throw debby::error on storage error.
     */
    void wipe ();

    /**
     * Fetch all contacts and process them by @a f
     *
     * @throw debby::error on storage error.
     */
    void for_each (std::function<void(contact::contact const &)> f);

    /**
     * Fetch all contacts and process them by @a f until @f does not
     * return @c false.
     *
     * @throw debby::error on storage error.
     */
    void for_each_until (std::function<bool(contact::contact const &)> f);

    /**
     * Execute transaction (batch execution). Useful for storages that support
     * tranactions
     */
    bool transaction (std::function<bool()> op) noexcept;

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
