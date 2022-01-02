////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.02 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

template <typename ForwardIt, typename Op, typename OnFailure>
auto process_transaction (database_handle_t dbh
    , ForwardIt first
    , ForwardIt last
    , Op && op
    , OnFailure && on_failure) -> bool
{
    debby::error err;
    auto success = dbh->begin();

    if (success) {
        for (; first != last; ++first) {
            success = success && op(*first, & err);
        }
    }

    if (success)
        dbh->commit();
    else
        dbh->rollback();

    if (!success) {
        on_failure(& err);
    }

    return success;
}

}}} // namespace chat::persistent_storage::sqlite3
