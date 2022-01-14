////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.28 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include <vector>

namespace chat {

template <typename Impl>
class basic_group_list
{
public:
    std::size_t count () const
    {
        return static_cast<Impl const *>(this)->count_impl();
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
     * @return @c 1 if group successfully updated or @c 0 if group not found
     *     with @c contact_id or @c -1 on error.
     */
    int update (contact::group const & g, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->update_impl(g, perr);
    }

    /**
     * Get group by @a id.
     */
    pfs::optional<contact::group> get (contact::contact_id id, error * perr = nullptr) const
    {
        return static_cast<Impl const *>(this)->get_impl(id, perr);
    }

    /**
     * Adds member specified by @a member_id to the group specified by @a group_id.
     *
     * @return @c false on error or @c true if contact added was successfully
     *         or it is already a member of the specified group.
     */
    bool add_member (contact::contact_id group_id
        , contact::contact_id member_id
        , error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->add_member_impl(group_id, member_id, perr);
    }

    /**
     * Get members of the specified group.
     */
    std::vector<contact::contact> members (contact::contact_id group_id
        , error * perr = nullptr) const
    {
        return static_cast<Impl const *>(this)->members_impl(group_id, perr);
    }
};

} // namespace chat
