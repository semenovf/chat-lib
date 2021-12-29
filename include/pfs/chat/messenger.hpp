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
    , typename ContactListBuilder>
class messenger
{
public:
    using controller_type   = typename ControllerBuilder::type;
    using contact_list_type = typename ContactListBuilder::type;
//     using icon_library_type    = typename PersistentStorageAPI::icon_library_type;
//     using message_storage_type = typename PersistentStorageAPI::message_storage_type;
//     using media_cache_type     = typename PersistentStorageAPI::media_cache_type;

private:
    std::unique_ptr<controller_type>   _controller;
    std::shared_ptr<contact_list_type> _contact_list;

public:
    messenger ()
    {
        ControllerBuilder build_controller;
        _controller = build_controller();

        ContactListBuilder build_contact_list;
        _contact_list = build_contact_list();
    }

    ~messenger () = default;

    messenger (messenger const &) = delete;
    messenger & operator = (messenger const &) = delete;

    messenger (messenger &&) = delete;
    messenger & operator = (messenger &&) = delete;

    std::shared_ptr<contact_list_type> contact_list_shared () const noexcept
    {
        return _contact_list;
    }
};

} // namespace chat
