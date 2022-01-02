////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "strict_ptr_wrapper.hpp"
#include <memory>

namespace chat {

template <
      typename ControllerBuilder
    , typename ContactManagerBuilder
    , typename MessageStoreBuilder
    , typename DeliveryBuilder>
class messenger
{
public:
    using controller_type      = typename ControllerBuilder::type;
    using contact_manager_type = typename ContactManagerBuilder::type;
    using message_store_type   = typename MessageStoreBuilder::type;
//     using icon_library_type    = typename PersistentStorageAPI::icon_library_type;
//     using message_storage_type = typename PersistentStorageAPI::message_storage_type;
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
    }

    ~messenger () = default;

    messenger (messenger const &) = delete;
    messenger & operator = (messenger const &) = delete;

    messenger (messenger &&) = delete;
    messenger & operator = (messenger &&) = delete;

    strict_ptr_wrapper<contact_manager_type const> contact_manager () const noexcept
    {
        return strict_ptr_wrapper<contact_manager_type const>(*_contact_manager);
    }

    strict_ptr_wrapper<contact_manager_type> contact_manager () noexcept
    {
        return strict_ptr_wrapper<contact_manager_type>(*_contact_manager);
    }

    strict_ptr_wrapper<message_store_type const> message_store () const noexcept
    {
        return strict_ptr_wrapper<message_store_type const>(*_message_store);
    }

    strict_ptr_wrapper<message_store_type> message_store () noexcept
    {
        return strict_ptr_wrapper<message_store_type>(*_message_store);
    }
};

} // namespace chat
