////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.13 Initial version.
//      2021.12.27 Refactored.
//      2024.11.30 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "chat_impl.hpp"
#include "chat/message_store.hpp"
#include "chat/sqlite3.hpp"
#include <array>

namespace chat {

using relational_database_t = debby::relational_database<debby::backend_enum::sqlite3>;

namespace storage {

class sqlite3::message_store
{
public:
    relational_database_t * pdb {nullptr};
    contact::id me;

public:
    message_store (contact::id my_contact_id, relational_database_t & db)
        : pdb(& db)
        , me(my_contact_id)
    {}
};

sqlite3::message_store *
sqlite3::make_message_store (contact::id my_contact_id, debby::relational_database<debby::backend_enum::sqlite3> & db)
{
    return new sqlite3::message_store(my_contact_id, db);
}

} // namespace storage

using message_store_t = message_store<storage::sqlite3>;

template <>
message_store_t::message_store (rep * d) noexcept
   : _d(d)
{}

template <>
message_store_t::operator bool () const noexcept
{
    return !!_d;
}

template <> message_store_t::message_store (message_store && other) noexcept = default;
template <> message_store_t & message_store_t::operator = (message_store && other) noexcept = default;
template <> message_store_t::~message_store () = default;

template <>
message_store_t::chat_type message_store_t::open_chat (contact::id chat_id) const
{
    if (chat_id == contact::id{})
        return chat_type{};

    if (!_d)
        return chat_type{};

    if (_d->pdb == nullptr)
        return chat_type{};

    return chat_type{new storage::sqlite3::chat(_d->me, chat_id, *_d->pdb)};
}

template <>
void message_store_t::clear () noexcept
{
    auto tables = _d->pdb->tables("^" + storage::sqlite3::chat_table_name_prefix());

    if (tables.empty())
        return;

    _d->pdb->remove(tables);
}

} // namespace chat
