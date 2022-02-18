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
    using group_list_type   = typename Backend::group_list_type;

private:
    rep_type _rep;

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
     * Get contacts.
     */
    std::shared_ptr<contact_list_type> contacts () const noexcept;

    /**
     * Get groups.
     */
    std::shared_ptr<group_list_type> groups () const noexcept;

    /**
     * Wipes (erase all contacts, groups and channels) contact database.
     */
    bool wipe (error * perr = nullptr);

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
