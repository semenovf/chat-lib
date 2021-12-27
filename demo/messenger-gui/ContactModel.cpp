////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ContactModel.hpp"
#include "pfs/chat/contact.hpp"

#include <QDebug>

ContactModel::ContactModel (std::shared_ptr<ContactList> contactList, QObject * parent)
    : QAbstractTableModel(parent)
    , _contactList(contactList)
{}

int ContactModel::rowCount (QModelIndex const & parent) const
{
    Q_UNUSED(parent);
    qDebug() << "Row count:" << _contactList->count();
    return _contactList->count();
}

int ContactModel::columnCount (QModelIndex const & parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ContactModel::data (QModelIndex const & index, int role) const
{
    auto opt = _contactList->get(index.row());

    if (opt) {
        switch (role) {
            case Qt::DisplayRole:
            case AliasRole:
                qDebug() << "Contact found by row:" << index.row() << QString::fromStdString(opt->alias);
                return QString::fromStdString(opt->alias);
            case NameRole:
                return QString::fromStdString(opt->name);
            case LastActivityRole:
                return QString::fromStdString(to_string(opt->last_activity));

            default:
                break;
        }
    } else {
        qWarning() << "No contact found by row:" << index.row();
    }

    return QVariant{};
}

QHash<int, QByteArray> ContactModel::roleNames () const
{
    QHash<int, QByteArray> names;
    names[NameRole]         = "name";
    names[AliasRole]        = "alias";
    names[LastActivityRole] = "lastActivity";
    return names;
}
