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

message_store::message_store (database_handle_t dbh)
    : base_class()
    , _dbh(dbh)
{}

message_store::conversation_type
message_store::conversation_impl (contact::contact_id my_id
    , contact::contact_id addressee_id) const
{
    return conversation_type{my_id, addressee_id, _dbh};
}

}}} // namespace chat::persistent_storage::sqlite3
