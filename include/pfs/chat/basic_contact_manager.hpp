////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.28 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/strict_ptr_wrapper.hpp"
#include "pfs/optional.hpp"
#include "pfs/chat/contact.hpp"
#include <functional>

namespace chat {

template <typename Impl, typename Traits>
class basic_contact_manager
{
protected:
    using failure_handler_type = std::function<void(std::string const &)>;

public:
    using contact_list_type = typename Traits::contact_list_type;
    using group_list_type = typename Traits::group_list_type;

protected:
    failure_handler_type on_failure;

protected:
    basic_contact_manager (failure_handler_type f)
        : on_failure(f)
    {}

public:
    /**
     * Checks if contact manager opened/initialized successfully.
     */
    operator bool () const noexcept
    {
        return static_cast<Impl const *>(this)->ready();
    }

    /**
     * Wipes (erase all contacts, groups and channels) contact database.
     */
    bool wipe ()
    {
        return static_cast<Impl *>(this)->wipe_impl();
    }

    strict_ptr_wrapper<contact_list_type> contacts () noexcept
    {
        return static_cast<Impl *>(this)->contacts_impl();
    }

    strict_ptr_wrapper<contact_list_type const> contacts () const noexcept
    {
        return static_cast<Impl *>(this)->contacts_impl();
    }

    strict_ptr_wrapper<group_list_type> groups () noexcept
    {
        return static_cast<Impl *>(this)->groups_impl();
    }

    strict_ptr_wrapper<group_list_type const> groups () const noexcept
    {
        return static_cast<Impl *>(this)->groups_impl();
    }
};

} // namespace chat
