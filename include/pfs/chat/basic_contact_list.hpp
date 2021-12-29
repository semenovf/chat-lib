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
#include "pfs/optional.hpp"
#include "pfs/chat/contact.hpp"
#include <functional>

namespace chat {

template <typename Impl>
class basic_contact_list
{
protected:
    using failure_handler_type = std::function<void(std::string const &)>;

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
    int add (contact::contact const & c)
    {
        return static_cast<Impl *>(this)->add_impl(c);
    }

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::contact && c)
    {
        return static_cast<Impl *>(this)->add_impl(std::move(c));
    }

    /**
     * Adds series of contacts
     *
     * @return Total contacts added or -1 on error.
     */
    template <typename ForwardIt>
    int add (ForwardIt first, ForwardIt last)
    {
        return static_cast<Impl *>(this)->template add_impl<ForwardIt>(first, last);
    }

    /**
     * Adds group.
     *
     * @return @c 1 if group successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::group const & g)
    {
        return static_cast<Impl *>(this)->add_impl(g);
    }

    /**
     * Adds group.
     *
     * @return @c 1 if group successfully added or @c 0 if group already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::group && g)
    {
        return static_cast<Impl *>(this)->add_impl(std::move(g));
    }

    /**
     * @return @c 1 if contact successfully updated or @c 0 if contact not found
     *         with @c contact_id or @c -1 on error.
     */
    int update (contact::contact const & c)
    {
        return static_cast<Impl *>(this)->update_impl(c);
    }

    /**
     * @return @c 1 if group successfully updated or @c 0 if group not found
     *     with @c contact_id or @c -1 on error.
     */
    int update (contact::group const & g)
    {
        return static_cast<Impl *>(this)->update_impl(g);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact> get (contact::contact_id id)
    {
        return static_cast<Impl *>(this)->get_impl(id);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact> get (int offset)
    {
        return static_cast<Impl *>(this)->get_impl(offset);
    }

    /**
     * Fetch all contacts and process them by @a f
     */
    void all_of (std::function<void(contact::contact const &)> f)
    {
        static_cast<Impl *>(this)->all_of_impl(f);
    }
};

} // namespace chat

