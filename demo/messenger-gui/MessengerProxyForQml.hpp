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
#include <memory>

class MessengerProxyForQml : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ContactModel * contactModel READ contactModel CONSTANT)

private:
    ContactModel * contactModel ()
    {
        return & *_contactModel;
    }

private:
    std::shared_ptr<ContactModel> _contactModel;

public:
    MessengerProxyForQml (std::shared_ptr<ContactModel> contactModel
        , QObject * parent = nullptr);
};
