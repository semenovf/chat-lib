////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.24 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "flags.hpp"
#include <functional>

namespace chat {

enum class contact_sort_flag: int
{
      by_nothing = 0
    , no_order = 0
    , by_alias = 1 << 0

    , ascending_order  = 1 << 8
    , descending_order = 1 << 9
};

template <typename Backend>
class contact_list final
{
    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    contact_list () = default;
    contact_list (rep_type && rep);
    contact_list (contact_list const & other) = delete;
    contact_list & operator = (contact_list const & other) = delete;
    contact_list & operator = (contact_list && other) = delete;

public:
    contact_list (contact_list && other) = default;
    ~contact_list () = default;

public:
    /**
     */
    std::size_t count () const;

    /**
     */
    std::size_t count (conversation_enum type) const;

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

    /**
     * Removes contact from contact list.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    void remove (contact::id id);

    /**
     * Get contact by @a id. On error returns invalid contact.
     *
     * @return Contact with @a id or invalid contact if not found.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    contact::contact get (contact::id id) const;

    /**
     * Get contact by @a offset. On error returns invalid contact.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    //contact::contact get (int offset, int sf = sort_flags(contact_sort_flag::by_alias
    //    , contact_sort_flag::ascending_order)) const;
    contact::contact get (int offset, int sf = sort_flags(contact_sort_flag::by_nothing
        , contact_sort_flag::no_order)) const;

    /**
     * Fetch all contacts and process them by @a f
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    void for_each (std::function<void(contact::contact const &)> f);

    /**
     * Fetch all contacts and process them by @a f until @f does not
     * return @c false.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    void for_each_until (std::function<bool(contact::contact const &)> f);

public:
    template <typename ...Args>
    static contact_list make (Args &&... args)
    {
        return contact_list{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
