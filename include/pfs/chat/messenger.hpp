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

template <
      typename ControllerBuilder
    , typename ContactManagerBuilder
    , typename MessageStoreBuilder
    /*, typename DeliveryBuilder*/>
class messenger
{
public:
    using controller_type      = typename ControllerBuilder::type;
    using contact_manager_type = typename ContactManagerBuilder::type;
    using message_store_type   = typename MessageStoreBuilder::type;
//     using icon_library_type    = typename PersistentStorageAPI::icon_library_type;
//     using media_cache_type     = typename PersistentStorageAPI::media_cache_type;

private:
    std::unique_ptr<controller_type>      _controller;
    std::unique_ptr<contact_manager_type> _contact_manager;
    std::unique_ptr<message_store_type>   _message_store;

private:
    bool add_contact (contact::contact_id const & id
        , std::string const & alias
        , contact::type_enum type
        , bool force_update)
    {
        contact::contact c;
        error err;
        c.id    = id;
        c.alias = alias;
        c.type  = type;

        auto rc = _contact_manager->contacts().add(std::move(c), & err);

        if (rc > 0) {
            // Contact added successfully
            return true;
        } else if (rc == 0) {
            // Contact already exists

            if (force_update) {
                rc = _contact_manager->contacts().update(std::move(c), & err);

                if (rc > 0) {
                    return true;
                } else if (rc == 0) {
                    ; // Unexpected state (contact existence checked before)
                } else {
                    _controller->failure(err.what());
                }
            } else {
                _controller->failure(fmt::format("contact already exists: {} (#{})"
                    , alias
                    , to_string(id)));
            }
        } else {
            // Error
            _controller->failure(err.what());
        }

        return false;
    }

public:
    messenger ()
    {
        ControllerBuilder build_controller;
        _controller = build_controller();

        ContactManagerBuilder build_contact_manager;
        _contact_manager = build_contact_manager();

        MessageStoreBuilder build_message_store;
        _message_store = build_message_store();
    }

    ~messenger () = default;

    messenger (messenger const &) = delete;
    messenger & operator = (messenger const &) = delete;

    messenger (messenger &&) = delete;
    messenger & operator = (messenger &&) = delete;

    std::size_t contacts_count () const
    {
        return _contact_manager->contacts().count();
    }

    std::size_t contacts_count (contact::type_enum type) const
    {
        return _contact_manager->contacts().count(type);
    }

    /**
     * Add contact
     */
    bool add_contact (contact::contact_id const & id
        , std::string const & alias
        , contact::type_enum type)
    {
        return add_contact(id, alias, type, false);
    }

    /**
     * Add or update contact
     */
    bool add_or_update_contact (contact::contact_id const & id
        , std::string const & alias
        , contact::type_enum type)
    {
        return add_contact(id, alias, type, true);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact> get_contact (contact::contact_id id) const
    {
        error err;
        auto result = _contact_manager->contacts().get(id, & err);

        if (err)
            _controller->failure(err.what());

        return result;
    }

    /**
     * Get contact by offset.
     */
    pfs::optional<contact::contact> get_contact (int offset) const
    {
        error err;
        auto result = _contact_manager->contacts().get(offset, & err);

        if (err)
            _controller->failure(err.what());

        return result;
    }

    template <typename F>
    void for_each_contact (F && f) const
    {
        auto contacts = _contact_manager->contacts();

        for (auto c: contacts) {
            f(c);
        }
    }
};

} // namespace chat
