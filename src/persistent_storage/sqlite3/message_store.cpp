////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.13 Initial version.
//      2021.12.27 Refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/persistent_storage/sqlite3/message_store.hpp"
#include "pfs/chat/persistent_storage/sqlite3/transaction.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <array>
#include <cassert>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;

message_store::message_store (database_handle_t dbh, failure_handler_type f)
    : base_class(f)
    , _dbh(dbh)
{}

auto message_store::begin_conversation_impl (contact::contact_id c)
    -> conversation_type &
{
    auto res = _conversation_cache.emplace(c, conversation_type{c, _dbh, on_failure});
    return res.first->second;
}

}}} // namespace chat::persistent_storage::sqlite3
