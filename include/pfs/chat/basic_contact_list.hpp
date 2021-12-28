////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_channel.hpp"
#include "basic_contact_list.hpp"
#include "basic_group.hpp"
#include "pfs/optional.hpp"
#include "pfs/chat/contact.hpp"
#include <functional>

namespace chat {

template <typename Impl>
class basic_contact_list
{
protected:
    using failure_handler_type = std::function<void(std::string const &)>;

protected:
    basic_contact_list (failure_handler_type f)
        : on_failure(f)
    {}

    basic_contact_list () = default;
    ~basic_contact_list () = default;
    basic_contact_list (basic_contact_list const &) = delete;
    basic_contact_list & operator = (basic_contact_list const &) = delete;
    basic_contact_list (basic_contact_list && other) = default;
    basic_contact_list & operator = (basic_contact_list && other) = default;

protected:
    failure_handler_type on_failure;

public:
    /**
     * Checks if contact list is open.
     */
    operator bool () const noexcept
    {
        return static_cast<Impl const *>(this)->is_opened();
    }

    std::size_t count () const
    {
        return static_cast<Impl const *>(this)->count_impl();
    }

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::contact_credentials const & c)
    {
        return static_cast<Impl *>(this)->add_impl(c);
    }

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::contact_credentials && c)
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
     * @return @c 1 if contact successfully updated or @c 0 if contact not found
     *         with @c contact_id or @c -1 on error.
     */
    int update (contact::contact_credentials const & c)
    {
        return static_cast<Impl *>(this)->update_impl(c);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact_credentials> get (contact::contact_id id)
    {
        return static_cast<Impl *>(this)->get_impl(id);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact_credentials> get (int offset)
    {
        return static_cast<Impl *>(this)->get_impl(offset);
    }

    /**
     * Fetch all contacts and process them by @a f
     */
    void all_of (std::function<void(contact::contact_credentials const &)> f)
    {
        static_cast<Impl *>(this)->all_of_impl(f);
    }

    /**
     * Wipes (erase all contacts) contact list.
     */
    bool wipe ()
    {
        return static_cast<Impl *>(this)->wipe_impl();
    }
};

} // namespace chat

