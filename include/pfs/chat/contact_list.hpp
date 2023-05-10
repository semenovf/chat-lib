////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.24 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "exports.hpp"
#include "pfs/string_view.hpp"
#include <algorithm>
#include <functional>
#include <vector>

namespace chat {

template <typename Backend>
class contact_list final
{
public:
    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    contact_list () = default;

public:
    contact_list (rep_type && rep);
    contact_list (contact_list const & other) = delete;
    contact_list (contact_list && other) = default;
    contact_list & operator = (contact_list const & other) = delete;
    contact_list & operator = (contact_list && other) = default;
    ~contact_list () = default;

public:
    /**
     * Count of contacts in this contact list.
     */
    CHAT__EXPORT std::size_t count () const;

    /**
     * Count of contacts of specified type in this contact list.
     */
    CHAT__EXPORT std::size_t count (conversation_enum type) const;

    /**
     * Get contact by @a id. On error returns invalid contact.
     *
     * @return Contact with @a id or invalid contact if not found.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT contact::contact get (contact::id id) const;

    /**
     * Get contact by @a index. On error returns invalid contact.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT contact::contact at (int index) const;

    /**
     * Fetch all contacts and process them by @a f
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT void for_each (std::function<void(contact::contact const &)> f) const;

    /**
     * Fetch all contacts and process them by @a f until @f does not
     * return @c false.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT void for_each_until (std::function<bool(contact::contact const &)> f) const;

public:
    template <typename ...Args>
    static contact_list make (Args &&... args)
    {
        return contact_list{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
