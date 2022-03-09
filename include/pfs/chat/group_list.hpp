////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.28 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include <vector>

namespace chat {

template <typename Backend>
class group_list final
{
    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    group_list () = default;
    group_list (rep_type && rep);
    group_list (group_list const & other) = delete;
    group_list & operator = (group_list const & other) = delete;
    group_list & operator = (group_list && other) = delete;

public:
    group_list (group_list && other) = default;
    ~group_list () = default;

public:
    /**
     */
    std::size_t count () const;

    /**
     * Adds group.
     *
     * @return @c 1 if group successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::group const & g, error * perr = nullptr);

    /**
     * @return @c 1 if group successfully updated or @c 0 if group not found
     *     with @c contact_id or @c -1 on error.
     */
    int update (contact::group const & g, error * perr = nullptr);

    /**
     * Get group by @a id.
     */
    contact::group get (contact::contact_id id, error * perr = nullptr) const;

    /**
     * Get group by @a id.
     */
    contact::contact get_or (contact::contact_id id, contact::group const & default_value) const
    {
        error err;
        auto c = get(id, & err);
        return err ? default_value : c;
    }

    /**
     * Adds member specified by @a member_id to the group specified by @a group_id.
     *
     * @return @c false on error or @c true if contact added was successfully
     *         or it is already a member of the specified group.
     */
    bool add_member (contact::contact_id group_id
        , contact::contact_id member_id
        , error * perr = nullptr);

    /**
     * Get members of the specified group.
     */
    std::vector<contact::contact> members (contact::contact_id group_id
        , error * perr = nullptr) const;

    /**
     * Checks if contact @a member_id is the member of group @a group_id.
     */
    bool is_member_of (contact::contact_id member_id, contact::contact_id group_id) const;

public:
    template <typename ...Args>
    static group_list make (Args &&... args)
    {
        return group_list{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
