
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.03 Initial version.
//      2021.11.21 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"
#include "pfs/chat/basic_message_store.hpp"
#include "pfs/chat/exports.hpp"
#include <string>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

class message_store: public basic_message_store<message_store>
{
    friend class basic_message_store<message_store>;

    using base_class = basic_message_store<message_store>;
    using failure_handler_type = typename base_class::failure_handler_type;

public:
    enum class route_enum {
          incoming = 1
        , outgoing
    };

private:
    database_handle_t _dbh;
    std::string _table_name;
    route_enum _route {route_enum::incoming};

protected:
    bool is_opened () const noexcept
    {
        return !!_dbh;
    }

//     std::size_t count_impl () const;
    int add_impl (message::credentials const & m);
    int add_impl (message::credentials && m);
//     int update_impl (contact::contact const & c);
    std::vector<message::credentials> get_impl (message::message_id id);
//     pfs::optional<contact::contact> get_impl (int offset);
//     bool all_of_impl (std::function<void(contact::contact const &)> f);
    bool wipe_impl ();

    bool fill_credentials (result_t * res, message::credentials * m);
//     int prefetch (int offset, int limit);


public:
    message_store (route_enum route
        , database_handle_t dbh
        , std::function<void(std::string const &)> f);

    ~message_store ()
    {
        database_handle_t empty;
        _dbh.swap(empty);
    }

    message_store (message_store && other)
    {
        *this = std::move(other);
    }

    message_store & operator = (message_store && other)
    {
        this->~message_store();
        on_failure = std::move(other.on_failure);
        _dbh = std::move(other._dbh);
        _table_name = std::move(other._table_name);
        other.~message_store();
        return *this;
    }
};

}}} // namespace chat::persistent_storage::sqlite3
