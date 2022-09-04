////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.13 Initial version.
//      2021.12.27 Refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/backend/sqlite3/message_store.hpp"
#include "pfs/debby/backend/sqlite3/time_point_traits.hpp"
#include "pfs/debby/backend/sqlite3/uuid_traits.hpp"
#include <array>

namespace chat {

// NOTE Must be synchronized with analog in conversation.cpp
static std::string const DEFAULT_TABLE_NAME_PREFIX { "#" };

namespace backend {
namespace sqlite3 {

message_store::rep_type
message_store::make (contact::id me, shared_db_handle dbh)
{
    rep_type rep;
    rep.dbh = dbh;
    rep.me  = me;
    return rep;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::message_store;

template <>
message_store<BACKEND>::message_store (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
message_store<BACKEND>::operator bool () const noexcept
{
    return !!_rep.dbh;
}

template <>
message_store<BACKEND>::conversation_type
message_store<BACKEND>::conversation (contact::id conversation_id) const
{
    if (conversation_id == contact::id{})
        return conversation_type{};

    return conversation_type::make(_rep.me, conversation_id, _rep.dbh);
}

template <>
void
message_store<BACKEND>::wipe () noexcept
{
    auto tables = _rep.dbh->tables("^" + DEFAULT_TABLE_NAME_PREFIX);

    if (tables.empty())
        return;

    _rep.dbh->remove(tables);
}

} // namespace chat
