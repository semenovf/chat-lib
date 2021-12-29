////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "database_traits.hpp"
#include "pfs/string_view.hpp"
#include "pfs/chat/basic_contact_list.hpp"
#include "pfs/chat/contact.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace chat {
namespace persistent_storage {
namespace sqlite3 {

class contact_manager;

class contact_list final: public basic_contact_list<contact_list>
{
    friend class basic_contact_list<contact_list>;
    friend class contact_manager;

    using base_class = basic_contact_list<contact_list>;

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
    in_memory_cache   _cache;
    std::string const & _table_name;
    failure_handler_t & _on_failure;

protected:
    std::size_t count_impl () const;
    std::size_t count_impl (contact::type_enum type) const;

    int add_impl (contact::contact const & c);
    int update_impl (contact::contact const & c);

    int add_impl (contact::group const & g)
    {
        contact::contact c {g.id, g.alias, chat::contact::type_enum::group};
        return add_impl(c);
    }

    int add_impl (contact::contact && c)
    {
        return add_impl(c);
    }

    int add_impl (contact::group && g)
    {
        return add_impl(g);
    }

    int update_impl (contact::group const & g)
    {
        contact::contact c {g.id, g.alias, chat::contact::type_enum::group};
        return update_impl(c);
    }

    pfs::optional<contact::contact> get_impl (contact::contact_id id);
    pfs::optional<contact::contact> get_impl (int offset);
    bool all_of_impl (std::function<void(contact::contact const &)> f);

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

private:
    contact_list () = delete;
    ~contact_list () = default;
    contact_list (contact_list const & other) = delete;
    contact_list & operator = (contact_list const & other) = delete;
    contact_list (contact_list && other) = delete;
    contact_list & operator = (contact_list && other) = delete;

    contact_list (database_handle_t dbh
        , std::string const & table_name
        , failure_handler_t & f);
};

}}} // namespace chat::persistent_storage::sqlite3

