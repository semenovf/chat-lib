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
      by_alias = 1 << 0

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
    std::size_t count (contact::type_enum type) const;

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id.
     *
     * @throw debby::error on storage error.
     */
    int add (contact::contact const & c);

    /**
     * @return @c 1 if contact successfully updated or @c 0 if contact not found
     *         with @c contact_id.
     *
     * @throw debby::error on storage error.
     */
    int update (contact::contact const & c);

    /**
     * Removes contact from contact list.
     *
     * @throw debby::error on storage error.
     */
    void remove (contact::contact_id id);

    /**
     * Get contact by @a id. On error returns invalid contact.
     *
     * @return Contact with @a id or invalid contact if not found.
     *
     * @throw debby::error on storage error.
     */
    contact::contact get (contact::contact_id id) const;

    /**
     * Get contact by @a offset. On error returns invalid contact.
     *
     * @return Contact with @a id or invalid contact if not found.
     *
     * @throw debby::error on storage error.
     */
    contact::contact get (int offset, int sf = sort_flags(contact_sort_flag::by_alias
        , contact_sort_flag::ascending_order)) const;

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

public:
    template <typename ...Args>
    static contact_list make (Args &&... args)
    {
        return contact_list{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
