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

    auto contact_manager () const noexcept -> contact_manager_type const &
    {
        return *_contact_manager;
    }

    auto contact_manager () noexcept -> contact_manager_type &
    {
        return *_contact_manager;
    }

    auto message_store () const noexcept -> message_store_type const &
    {
        return *_message_store;
    }

    auto message_store () noexcept -> message_store_type &
    {
        return *_message_store;
    }
};

} // namespace chat
