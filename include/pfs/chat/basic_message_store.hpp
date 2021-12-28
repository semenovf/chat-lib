////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/optional.hpp"
#include "pfs/chat/message.hpp"
#include <functional>

namespace chat {
namespace persistent_storage {

template <typename Impl>
class basic_message_store
{
protected:
    using failure_handler_type = std::function<void(std::string const &)>;

protected:
    basic_message_store (failure_handler_type f)
        : on_failure(f)
    {}

    basic_message_store () = default;
    ~basic_message_store () = default;
    basic_message_store (basic_message_store const &) = delete;
    basic_message_store & operator = (basic_message_store const &) = delete;
    basic_message_store (basic_message_store && other) = default;
    basic_message_store & operator = (basic_message_store && other) = default;

protected:
    failure_handler_type on_failure;

public:
    /**
     * Checks if message store is open.
     */
    operator bool () const noexcept
    {
        return static_cast<Impl const *>(this)->is_opened();
    }

//     std::size_t count () const
//     {
//         return static_cast<Impl const *>(this)->count_impl();
//     }

    /**
     * Adds message.
     *
     * @return @c 1 if message successfully added or @c 0 if message already
     *         exists with @c message_id or @c -1 on error.
     */
    int add (message::credentials const & m)
    {
        return static_cast<Impl *>(this)->add_impl(m);
    }

    /**
     * Adds message.
     *
     * @return @c 1 if message successfully added or @c 0 if message already
     *         exists with @c message_id or @c -1 on error.
     */
    int add (message::credentials && m)
    {
        return static_cast<Impl *>(this)->add_impl(std::move(m));
    }

//     /**
//      * @return @c 1 if contact successfully updated or @c 0 if contact not found
//      *         with @c contact_id or @c -1 on error.
//      */
//     int update (contact::contact const & c)
//     {
//         return static_cast<Impl *>(this)->update_impl(c);
//     }


    /**
     * Get message by @a id
     *
     * @details For incoming messages and individual outgoing messages loads
     *          unique message credentials.
     *          For outgoing group message loads message credentials for group
     *          message and appropriate individual messages.
     */
    std::vector<message::credentials> get (message::message_id id)
    {
        return static_cast<Impl *>(this)->get_impl(id);
    }

//     /**
//      * Get contact by @a id.
//      */
//     pfs::optional<contact::contact> get (int offset)
//     {
//         return static_cast<Impl *>(this)->get_impl(offset);
//     }
//
//     /**
//      * Fetch all contacts and process them by @a f
//      */
//     void all_of (std::function<void(contact::contact const &)> f)
//     {
//         static_cast<Impl *>(this)->all_of_impl(f);
//     }

    /**
     * Wipes (erases all) messages.
     */
    bool wipe ()
    {
        return static_cast<Impl *>(this)->wipe_impl();
    }
};

}} // namespace chat::persistent_storage


