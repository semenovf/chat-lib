////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include "pfs/optional.hpp"

namespace chat {

template <typename Impl>
class basic_contact_list
{
public:
    std::size_t count () const
    {
        return static_cast<Impl const *>(this)->count_impl();
    }

    std::size_t count (contact::type_enum type) const
    {
        return static_cast<Impl const *>(this)->count_impl(type);
    }

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::contact const & c, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->add_impl(c, perr);
    }

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::contact && c, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->add_impl(std::move(c), perr);
    }

    /**
     * Adds series of contacts
     *
     * @return Total contacts added or -1 on error.
     */
    template <typename ForwardIt>
    int add (ForwardIt first, ForwardIt last, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->template add_impl<ForwardIt>(first, last, perr);
    }

    /**
     * Adds group.
     *
     * @return @c 1 if group successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::group const & g, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->add_impl(g, perr);
    }

    /**
     * Adds group.
     *
     * @return @c 1 if group successfully added or @c 0 if group already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::group && g, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->add_impl(std::move(g), perr);
    }

    /**
     * @return @c 1 if contact successfully updated or @c 0 if contact not found
     *         with @c contact_id or @c -1 on error.
     */
    int update (contact::contact const & c, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->update_impl(c, perr);
    }

    /**
     * @return @c 1 if group successfully updated or @c 0 if group not found
     *     with @c contact_id or @c -1 on error.
     */
    int update (contact::group const & g, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->update_impl(g, perr);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact> get (contact::contact_id id, error * perr = nullptr) const
    {
        return static_cast<Impl const *>(this)->get_impl(id, perr);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact> get (int offset, error * perr = nullptr) const
    {
        return static_cast<Impl const *>(this)->get_impl(offset, perr);
    }

    /**
     * Fetch all contacts and process them by @a f
     */
    void all_of (std::function<void(contact::contact const &)> f, error * perr = nullptr)
    {
        static_cast<Impl *>(this)->all_of_impl(f, perr);
    }
};

} // namespace chat
