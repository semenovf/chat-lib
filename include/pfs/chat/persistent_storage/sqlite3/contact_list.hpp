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
    mutable in_memory_cache _cache;
    std::string const & _table_name;

protected:
    std::size_t count_impl () const;
    std::size_t count_impl (contact::type_enum type) const;

    int add_impl (contact::contact const & c, error * perr);
    int update_impl (contact::contact const & c, error * perr);

    int add_impl (contact::group const & g, error * perr)
    {
        contact::contact c {g.id, g.alias, g.avatar, chat::contact::type_enum::group};
        return add_impl(c, perr);
    }

    int add_impl (contact::contact && c, error * perr)
    {
        return add_impl(c, perr);
    }

    int add_impl (contact::group && g, error * perr)
    {
        return add_impl(g, perr);
    }

    int update_impl (contact::group const & g, error * perr)
    {
        contact::contact c {g.id, g.alias, g.avatar, chat::contact::type_enum::group};
        return update_impl(c, perr);
    }

    pfs::optional<contact::contact> get_impl (contact::contact_id id, error * perr) const;
    pfs::optional<contact::contact> get_impl (int offset, error * perr) const;
    bool all_of_impl (std::function<void(contact::contact const &)> f
        , error * perr);

    bool fill_contact (result_t * res, contact::contact * c) const;
    void invalidate_cache ();
    bool prefetch (int offset, int limit, error * perr) const;

    template <typename ForwardIt>
    int add_impl (ForwardIt first, ForwardIt last, error * perr)
    {
        int counter = 0;
        bool success = _dbh->begin();

        if (success) {
            for (int i = 0; first != last; ++first, i++) {
                auto n = add_impl(*first, perr);
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
    contact_list (contact_list const & other) = delete;
    contact_list (contact_list && other) = delete;
    contact_list & operator = (contact_list const & other) = delete;
    contact_list & operator = (contact_list && other) = delete;

    contact_list (database_handle_t dbh, std::string const & table_name);

public:
    ~contact_list () = default;
};

}}} // namespace chat::persistent_storage::sqlite3

