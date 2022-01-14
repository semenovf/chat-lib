////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Messenger.hpp"
#include <QAbstractTableModel>
#include <QHash>
#include <QVariant>

namespace ui {
namespace qt {

class ContactModel : public QAbstractTableModel
{
    enum Role {
        AliasRole = Qt::UserRole
    };

private:
    SharedMessenger _messenger;

public:
    ContactModel (SharedMessenger messenger, QObject * parent = nullptr);

    Q_INVOKABLE int rowCount (QModelIndex const & parent = QModelIndex()) const override;
    Q_INVOKABLE int columnCount (QModelIndex const & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data (QModelIndex const & index, int role) const override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames () const override;
};

}} // namespace ui::qt

// Q_DECLARE_METATYPE(ui::qt::ContactModel*)
