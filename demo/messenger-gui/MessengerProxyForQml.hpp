////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <QObject>
#include "ContactModel.hpp"
#include "ConversationModel.hpp"
#include <memory>

namespace ui {
namespace qt {

class MessengerProxyForQml : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ContactModel * contactModel READ contactModel CONSTANT)
    Q_PROPERTY(ConversationModel * conversationModel READ conversationModel CONSTANT)

private:
    ContactModel * contactModel ()
    {
        return & _contactModel;
    }

    ConversationModel * conversationModel ()
    {
        return & _conversationModel;
    }

private:
    ContactModel      _contactModel;
    ConversationModel _conversationModel;

public:
    MessengerProxyForQml (SharedMessenger messenger, QObject * parent = nullptr);
};

}} // namespace ui::qt
