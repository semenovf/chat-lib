////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <QAbstractTableModel>
#include <QByteArray>
#include <QHash>

class ConversationModel : public QAbstractTableModel
{
    Q_OBJECT
//     Q_PROPERTY(QString recipient READ recipient WRITE setRecipient NOTIFY recipientChanged)

public:
    ConversationModel (QObject * parent = nullptr);

//     QString recipient() const;
//     void setRecipient(const QString &recipient);

    Q_INVOKABLE int rowCount (QModelIndex const & parent = QModelIndex()) const override;
    Q_INVOKABLE int columnCount (QModelIndex const & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data (QModelIndex const & index, int role) const override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames () const override;

//     Q_INVOKABLE void sendMessage(const QString &recipient, const QString &message);

public:
//     Q_SIGNAL recipientChanged();

private:
//     QString m_recipient;
};
