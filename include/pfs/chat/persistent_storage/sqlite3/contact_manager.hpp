////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"
#include "pfs/chat/basic_contact_manager.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/exports.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

// struct entity_storage_traits
// {
//     using database_type   = debby::sqlite3::database;
//     using database_handle = std::shared_ptr<database_type>;
//     using result_type     = debby::sqlite3::result;
//     using statement_type  = debby::sqlite3::statement;
// };

class contact_manager: public basic_contact_manager<contact_manager>
{
    friend class basic_contact_manager<contact_manager>;

    using base_class = basic_contact_manager<contact_manager>;
    using failure_handler_type = typename base_class::failure_handler_type;

    struct in_memory_cache
    {
        int offset;
        int limit;
        bool dirty;
        std::vector<contact::contact_credentials> data;
        std::map<contact::contact_id, std::size_t> map;
    };

private:
    database_handle_t _dbh;
    std::string _contacts_table_name;
    std::string _members_table_name;
    std::string _followers_table_name;
//     in_memory_cache _cache;

protected:
    bool ready () const noexcept
    {
        return !!_dbh;
    }

//     std::size_t count_impl () const;
//     int add_impl (contact::contact_credentials const & c);
//     int add_impl (contact::contact_credentials && c);
//     int update_impl (contact::contact_credentials const & c);
//     pfs::optional<contact::contact_credentials> get_impl (contact::contact_id id);
//     pfs::optional<contact::contact_credentials> get_impl (int offset);
//     bool all_of_impl (std::function<void(contact::contact_credentials const &)> f);
    bool wipe_impl ();

//     bool fill_contact (result_t * res, contact::contact_credentials * c);
//     void invalidate_cache ();
//     int prefetch (int offset, int limit);
//
//     template <typename ForwardIt>
//     int add_impl (ForwardIt first, ForwardIt last)
//     {
//         int counter = 0;
//         bool success = _dbh->begin();
//
//         if (success) {
//             for (int i = 0; first != last; ++first, i++) {
//                 auto n = add_impl(*first);
//                 counter += n > 0 ? 1 : 0;
//             }
//         }
//
//         if (success) {
//             _dbh->commit();
//         } else {
//             _dbh->rollback();
//             counter = -1;
//         }
//
//         return counter;
//     }

public:
    contact_manager (database_handle_t dbh, std::function<void(std::string const &)> f);

    ~contact_manager ()
    {
        database_handle_t empty;
        _dbh.swap(empty);
    }

    contact_manager (contact_manager && other)
    {
        *this = std::move(other);
    }

    contact_manager & operator = (contact_manager && other)
    {
        this->~contact_manager();
        on_failure = std::move(other.on_failure);
        _dbh = std::move(other._dbh);
        _contacts_table_name  = std::move(other._contacts_table_name);
        _members_table_name   = std::move(other._members_table_name);
        _followers_table_name = std::move(other._followers_table_name);
        // FIXME
        //_cache = std::move(other._cache);
        other.~contact_manager();
        return *this;
    }
};

}}} // namespace chat::persistent_storage::sqlite3
