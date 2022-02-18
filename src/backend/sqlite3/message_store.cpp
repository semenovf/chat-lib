////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.13 Initial version.
//      2021.12.27 Refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/backend/sqlite3/message_store.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <array>
#include <cassert>

namespace chat {

using namespace debby::sqlite3;

namespace {
    // NOTE Must be synchronized with analog in conversation.cpp
    std::string const DEFAULT_TABLE_NAME_PREFIX { "#" };

    std::string const WIPE_ALL_ERROR { "wipe all converstions failure: {}" };
}

namespace backend {
namespace sqlite3 {

message_store::rep_type
message_store::make (contact::contact_id me, shared_db_handle dbh, error *)
{
    rep_type rep;
    rep.dbh = dbh;
    rep.me  = me;
    return rep;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::message_store

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
message_store<BACKEND>::conversation (contact::contact_id addressee_id
    , error * perr) const
{
    if (addressee_id == contact::contact_id{}) {
        error err {errc::invalid_argument, "bad addressee identifier"};
        if (perr) *perr = err; else CHAT__THROW(err);
        return conversation_type{};
    }

    return conversation_type::make(_rep.me, addressee_id, _rep.dbh, perr);
}

template <>
bool
message_store<BACKEND>::wipe (error * perr) noexcept
{
    debby::error storage_err;
    auto tables = _rep.dbh->tables("^" + DEFAULT_TABLE_NAME_PREFIX, & storage_err);

    auto success = !storage_err;

    if (success) {
        if (tables.empty())
            return true;

        success = _rep.dbh->remove(tables, & storage_err);
    }

    if (!success) {
        error err {errc::storage_error, fmt::format(WIPE_ALL_ERROR, storage_err.what())};
        if (perr) *perr = err;
    }

    return success;
}

} // namespace chat
