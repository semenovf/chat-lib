////////////////////////////////////////////////////////////////////////////////
// TacticalPad3
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// Changelog:
//      2021.12.12 Исходная версия.
////////////////////////////////////////////////////////////////////////////////
#include "ContactModel.hpp"
#include "pfs/chat/contact.hpp"

#include <QDebug>

namespace ui {
namespace qt {

ContactModel::ContactModel (SharedMessenger messenger, QObject * parent)
    : QAbstractTableModel(parent)
    , _messenger(messenger)
{}

int ContactModel::rowCount (QModelIndex const & parent) const
{
    Q_UNUSED(parent);

    auto const & contactList = _messenger->contact_manager().contacts();
    return contactList.count();
}

int ContactModel::columnCount (QModelIndex const & parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant ContactModel::data (QModelIndex const & index, int role) const
{
    auto const & contactList = _messenger->contact_manager().contacts();
    auto opt = contactList.get(index.row());

    if (opt) {
        switch (role) {
            case Qt::DisplayRole:
            case AliasRole:
                qDebug() << "ROLE:" << role << ":" << QString::fromStdString(opt->alias);
                return QString::fromStdString(opt->alias);

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
    names[AliasRole] = "alias";
    return names;
}

}} // namespace ui::qt
