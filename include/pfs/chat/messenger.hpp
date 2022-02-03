////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <cassert>

namespace chat {

template <typename MessengerBuilder>
class messenger
{
public:
    using controller_type      = typename MessengerBuilder::controller_type;
    using contact_manager_type = typename MessengerBuilder::contact_manager_type;
    using message_store_type   = typename MessengerBuilder::message_store_type;
//     using icon_library_type    = typename PersistentStorageAPI::icon_library_type;
//     using media_cache_type     = typename PersistentStorageAPI::media_cache_type;

private:
    std::unique_ptr<controller_type>      _controller;
    std::unique_ptr<contact_manager_type> _contact_manager;
    std::unique_ptr<message_store_type>   _message_store;

private:
    bool add_contact (contact::contact const & c, bool force_update)
    {
        error err;
        auto rc = _contact_manager->contacts().add(c, & err);

        if (rc > 0) {
            // Contact added successfully
            return true;
        } else if (rc == 0) {
            // Contact already exists

            if (force_update) {
                rc = _contact_manager->contacts().update(c, & err);

                if (rc > 0) {
                    return true;
                } else if (rc == 0) {
                    ; // Unexpected state (contact existence checked before)
                } else {
                    _controller->failure(err.what());
                }
            } else {
                _controller->failure(fmt::format("contact already exists: {} (#{})"
                    , c.alias
                    , to_string(c.id)));
            }
        } else {
            // Error
            _controller->failure(err.what());
        }

        return false;
    }

public:
    messenger (MessengerBuilder && builder)
        : _controller(builder.make_controller())
        , _contact_manager(builder.make_contact_manager())
        , _message_store(builder.make_message_store())
    {}

    ~messenger () = default;

    messenger (messenger const &) = delete;
    messenger & operator = (messenger const &) = delete;

    messenger (messenger &&) = delete;
    messenger & operator = (messenger &&) = delete;

    /**
     * Total contacts count.
     */
    auto contacts_count () const -> std::size_t
    {
        return _contact_manager->contacts().count();
    }

    /**
     * Total count of contacts with specified @a type.
     */
    auto contacts_count (contact::type_enum type) const -> std::size_t
    {
        return _contact_manager->contacts().count(type);
    }

    /**
     * Add contact
     */
    auto add_contact (contact::contact const & c) -> bool
    {
        return add_contact(c, false);
    }

    /**
     * Add or update contact
     */
    auto add_or_update_contact (contact::contact const & c) -> bool
    {
        return add_contact(c, true);
    }

    /**
     * Get contact by @a id.
     */
    auto get_contact (contact::contact_id id) const -> pfs::optional<contact::contact>
    {
        return _contact_manager->contacts().get(id);
    }

    /**
     * Get contact by offset.
     */
    auto get_contact (int offset) const -> pfs::optional<contact::contact>
    {
        return _contact_manager->contacts().get(offset);
    }

    template <typename F>
    void for_each_contact (F && f) const
    {
        auto contacts = _contact_manager->contacts();

        for (auto c: contacts) {
            f(c);
        }
    }

    auto unread_messages_count (contact::contact_id id) const -> std::size_t
    {
        auto conv = _message_store->conversation(id);
        return conv.unread_messages_count();
    }
};

} // namespace chat
