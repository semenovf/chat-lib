////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"
#include "pfs/chat/basic_contact_list.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/exports.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

class contact_list: public basic_contact_list<contact_list>
{
    friend class basic_contact_list<contact_list>;

    using base_class = basic_contact_list<contact_list>;
    using failure_handler_type = typename base_class::failure_handler_type;

    struct in_memory_cache
    {
        int offset;
        int limit;
        bool dirty;
        std::vector<contact::contact> data;
        std::map<contact::contact_id, std::size_t> map;
    };

private:
    database_handle_t _dbh;
    std::string _table_name;
    in_memory_cache _cache;

protected:
    bool is_opened () const noexcept
    {
        return !!_dbh;
    }

    std::size_t count_impl () const;
    int add_impl (contact::contact const & c);
    int add_impl (contact::contact && c);
    int update_impl (contact::contact const & c);
    pfs::optional<contact::contact> get_impl (contact::contact_id id);
    pfs::optional<contact::contact> get_impl (int offset);
    bool all_of_impl (std::function<void(contact::contact const &)> f);
    bool wipe_impl ();

    bool fill_contact (result_t * res, contact::contact * c);
    void invalidate_cache ();
    int prefetch (int offset, int limit);

    template <typename ForwardIt>
    int add_impl (ForwardIt first, ForwardIt last)
    {
        int counter = 0;
        bool success = _dbh->begin();

        if (success) {
            for (int i = 0; first != last; ++first, i++) {
                auto n = add_impl(*first);
                counter += n > 0 ? 1 : 0;
            }
        }

        if (success) {
            _dbh->commit();
        } else {
            _dbh->rollback();
            counter = -1;
        }

        return counter;
    }

public:
    contact_list (database_handle_t dbh, std::function<void(std::string const &)> f);

    ~contact_list ()
    {
        database_handle_t empty;
        _dbh.swap(empty);
    }

    contact_list (contact_list && other)
    {
        *this = std::move(other);
    }

    contact_list & operator = (contact_list && other)
    {
        this->~contact_list();
        on_failure = std::move(other.on_failure);
        _dbh = std::move(other._dbh);
        _table_name = std::move(other._table_name);
        _cache = std::move(other._cache);
        other.~contact_list();
        return *this;
    }
};

}}} // namespace chat::persistent_storage::sqlite3
