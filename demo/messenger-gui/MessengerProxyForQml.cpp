////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "MessengerProxyForQml.hpp"

MessengerProxyForQml::MessengerProxyForQml (std::shared_ptr<ContactModel> contactModel
    , QObject * parent)
    : QObject(parent)
    , _contactModel(contactModel)
{}
