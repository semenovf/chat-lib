////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.24 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include "exports.hpp"
#include <memory>
#include <functional>

namespace chat {

template <typename Storage>
class contact_list final
{
    using rep = typename Storage::contact_list;

private:
    std::unique_ptr<rep> _d;

public:
    CHAT__EXPORT contact_list ();
    CHAT__EXPORT contact_list (contact_list && other) noexcept;
    CHAT__EXPORT contact_list & operator = (contact_list && other) noexcept;
    CHAT__EXPORT ~contact_list ();

    // For internal use only
    CHAT__EXPORT contact_list (rep * d) noexcept;

    contact_list (contact_list const & other) = delete;
    contact_list & operator = (contact_list const & other) = delete;

public:
    // For internal use only
    bool add (contact::contact && c);

    /**
     * Count of contacts in this contact list.
     */
    CHAT__EXPORT std::size_t count () const;

    /**
     * Count of contacts of specified type in this contact list.
     */
    CHAT__EXPORT std::size_t count (chat_enum type) const;

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
};

} // namespace chat
