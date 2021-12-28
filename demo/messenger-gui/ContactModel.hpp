////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "ContactList.hpp"
#include <QAbstractTableModel>
#include <memory>

class ContactModel : public QAbstractTableModel
{
    enum Role {
        AliasRole = Qt::UserRole
    };

private:
    std::shared_ptr<ContactList> _contactList;

public:
    ContactModel (std::shared_ptr<ContactList> contactList, QObject * parent = nullptr);

    Q_INVOKABLE int rowCount (QModelIndex const & parent = QModelIndex()) const override;
    Q_INVOKABLE int columnCount (QModelIndex const & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data (QModelIndex const & index, int role) const override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames () const override;
};

Q_DECLARE_METATYPE(ContactModel*)
