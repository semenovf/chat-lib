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
         * Adds member specified by @a member_id to the group specified by @a group_id.
         *
         * @return @c false on error or @c true if contact added was successfully
         *         or it is already a member of the specified group.
         */
        bool add_member (contact::contact_id member_id, error * perr = nullptr);

        /**
         * Removes member from group.
         */
        bool remove_member (contact::contact_id member_id, error * perr = nullptr);

        /**
         * Get members of the specified group.
         */
        std::vector<contact::contact> members (error * perr = nullptr) const;

        /**
         * Checks if contact @a member_id is the member of group @a group_id.
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
    /**
     * Batch add series of contacts.
     *
     * @return Total contacts added or -1 on error.
     */
    int batch_add (std::function<bool()> has_next
        , std::function<contact::contact()> next
        , error * perr = nullptr);

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

    contact::contact my_contact () const;

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
    std::size_t group_count () const
    {
        return count(contact::type_enum::group);
    }

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::contact const & c, error * perr = nullptr);

    /**
     * Add person contact.
     */
    int add (contact::person const & p, error * perr = nullptr)
    {
        contact::contact c {
              p.id
            , p.alias
            , p.avatar
            , p.description
            , chat::contact::type_enum::person};
        return add(c, perr);
    }

    /**
     * Add group contact.
     */
    int add (contact::group const & g, error * perr = nullptr)
    {
        contact::contact c {
              g.id
            , g.alias
            , g.avatar
            , g.description
            , chat::contact::type_enum::group};
        return add(c, perr);
    }

    /**
     * Adds series of contacts.
     *
     * @return Total contacts added or -1 on error.
     */
    template <typename ForwardIt>
    int add (ForwardIt first, ForwardIt last, error * perr = nullptr)
    {
        return batch_add([& first, last] {return first != last; }
            , [& first] { return *first++; }
            , perr);
    }

    /**
     * Updates contact.
     *
     * @return @c 1 if contact successfully updated or @c 0 if contact not found
     *         with @c contact_id or @c -1 on error.
     */
    int update (contact::contact const & c, error * perr = nullptr);

    /**
     * Updates person contact.
     *
     * @return @c 1 if group successfully updated or @c 0 if group not found
     *     with @c contact_id or @c -1 on error.
     */
    int update (contact::person const & p, error * perr = nullptr)
    {
        contact::contact c {
              p.id
            , p.alias
            , p.avatar
            , p.description
            , chat::contact::type_enum::person };
        return update(c, perr);
    }

    /**
     * Updates group contact.
     *
     * @return @c 1 if group successfully updated or @c 0 if group not found
     *     with @c contact_id or @c -1 on error.
     */
    int update (contact::group const & g, error * perr = nullptr)
    {
        contact::contact c {
              g.id
            , g.alias
            , g.avatar
            , g.description
            , chat::contact::type_enum::group };
        return update(c, perr);
    }

    /**
     * Group reference if @a group_id is identifier of exist group or invalid
     * reference otherwise.
     */
    group_ref gref (contact::contact_id group_id);

    /**
     * Removes contact.
     *
     * @details If @a id is a group then group contact and all memberships
     *          will be removed. If @a id is a person contact membership will
     *          be removed in case of group participation.
     */
    bool remove (contact::contact_id id, error * perr = nullptr);

    /**
     * Get contact by @a id. On error returns invalid contact.
     */
    contact::contact get (contact::contact_id id, error * perr = nullptr) const;

    /**
     * Get contact by @a offset. On error returns invalid contact.
     */
    contact::contact get (int offset, error * perr = nullptr) const;

    /**
     * Wipes (erase all contacts, groups and channels) contact database.
     */
    bool wipe (error * perr = nullptr);

    /**
     * Fetch all contacts and process them by @a f
     *
     * @return @c true If no error occured or @c false otherwise.
     */
    bool for_each (std::function<void(contact::contact const &)> f, error * perr = nullptr);

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
