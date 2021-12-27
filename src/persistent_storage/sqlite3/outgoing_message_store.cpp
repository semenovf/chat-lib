////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
//      2021.12.13 `message_store` splitted into `incoming_message_store`
//                 and `outgoing_message_store`.
////////////////////////////////////////////////////////////////////////////////
#include "message_store.hpp"
#include "pfs/chat/persistent_storage/sqlite3/outgoing_mesage_store.hpp"
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <cassert>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

using namespace debby::sqlite3;

outgoing_message_store::outgoing_message_store ()
    : entity_storage()
{}

bool outgoing_message_store::open_impl (database_handle dbh, std::string const & table_name)
{
    message_store ms {dbh, table_name};
    return ms.open_outgoing();
}

void outgoing_message_store::close_impl ()
{
    message_store ms {_dbh, _table_name};
    ms.close();
}

void outgoing_message_store::wipe_impl ()
{
    message_store ms {_dbh, _table_name};
    ms.wipe();
}

CHAT__EXPORT bool outgoing_message_store::save (message::credentials const & m)
{
    message_store ms {_dbh, _table_name};
    return ms.save(m);
}

CHAT__EXPORT std::vector<message::credentials> outgoing_message_store::load (message::message_id id)
{
    message_store ms {_dbh, _table_name};
    return ms.load(id);
}

CHAT__EXPORT void outgoing_message_store::all_of (std::function<void(message::credentials const &)> f)
{
    message_store ms {_dbh, _table_name};
    ms.all_of(f);
}

}}} // namespace chat::persistent_storage::sqlite3

