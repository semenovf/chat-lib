////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "account.hpp"
#include "contact.hpp"
#include "message_id_generator.hpp"
#include "messenger_controller.hpp"
#include "pfs/net/p2p/controller.hpp"

namespace pfs {
namespace net {
namespace chat {

template <
      typename DispatchDeliveryController
    , typename PersistentStorageAPI>
class messenger
{
    using dispatch_delivery_controller = DispatchDeliveryController;
    using contact_list_type    = typename PersistentStorageAPI::contact_list_type;
    using icon_library_type    = typename PersistentStorageAPI::icon_library_type;
    using message_storage_type = typename PersistentStorageAPI::message_storage_type;
    using media_cache_type     = typename PersistentStorageAPI::media_cache_type;

private:
    messenger_controller * _pmc {nullptr};
    dispatch_delivery_controller * _pddc {nullptr};

public:
    static bool startup ()
    {
        return PersistentStorageAPI::startup();
    }

    static void cleanup ()
    {
        PersistentStorageAPI::cleanup();
    }

public:
    messenger (messenger_controller & mc, dispatch_delivery_controller & ddc)
        : _pmc(& mc)
        , _pddc(& ddc)
    {}

    messenger (messenger const &) = delete;
    messenger & operator = (messenger const &) = delete;

    messenger (messenger &&) = delete;
    messenger & operator = (messenger &&) = delete;
};

}}} // namespace pfs::net::chat
