////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "MessengerProxyForQml.hpp"

namespace ui {
namespace qt {

MessengerProxyForQml::MessengerProxyForQml (SharedMessenger messenger
    , QObject * parent)
    : QObject(parent)
    , _contactModel(messenger)
    , _conversationModel(messenger)
{}

}} // namespace ui::qt
