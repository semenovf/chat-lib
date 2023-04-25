////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.24 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "exports.hpp"
#include "pfs/string_view.hpp"
#include "pfs/unicode/search.hpp"
#include "pfs/unicode/utf8_iterator.hpp"
#include <algorithm>
#include <functional>
#include <vector>

namespace chat {

template <typename Backend>
class contact_list final
{
    using rep_type = typename Backend::rep_type;
    using utf8_input_iterator = pfs::unicode::utf8_input_iterator<std::string::const_iterator>;

private:
    rep_type _rep;

private:
    contact_list () = default;

public:
    contact_list (rep_type && rep);
    contact_list (contact_list const & other) = delete;
    contact_list (contact_list && other) = default;
    contact_list & operator = (contact_list const & other) = delete;
    contact_list & operator = (contact_list && other) = default;
    ~contact_list () = default;

public:
    /**
     * Count of contacts in this contact list.
     */
    CHAT__EXPORT std::size_t count () const;

    /**
     * Count of contacts of specified type in this contact list.
     */
    CHAT__EXPORT std::size_t count (conversation_enum type) const;

    /**
     * Get contact by @a id. On error returns invalid contact.
     *
     * @return Contact with @a id or invalid contact if not found.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT contact::contact get (contact::id id) const;

    /**
     * Get contact by @a index. On error returns invalid contact.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT contact::contact at (int index) const;

    /**
     * Fetch all contacts and process them by @a f
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT void for_each (std::function<void(contact::contact const &)> f) const;

    /**
     * Fetch all contacts and process them by @a f until @f does not
     * return @c false.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    CHAT__EXPORT void for_each_until (std::function<bool(contact::contact const &)> f) const;

public:
    ////////////////////////////////////////////////////////////////////////////
    // Search specific types and methods.
    ////////////////////////////////////////////////////////////////////////////
    enum search_flag
    {
          ignore_case = 1 << 0
        , alias_field = 1 << 1
        , desc_field  = 1 << 2
    };

    struct match_item
    {
        contact::id contact_id;
        search_flag field;      // field that matches search pattern (alias_field or desc_field)
        pfs::unicode::match_item m;
    };

    struct search_result
    {
        std::vector<match_item> m;
    };

private:
    static void search_all (std::string const & s, std::string const & pattern
        , bool ignore_case, std::function<void(pfs::unicode::match_item const &)> && f)
    {
        auto first = utf8_input_iterator::begin(s.begin(), s.end());
        auto s_first = utf8_input_iterator::begin(pattern.begin(), pattern.end());

        pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
            , ignore_case, std::move(f));
    }

public:
    /**
     * Searches contact list for specified @a pattern.
     */
    search_result search_all (std::string const & pattern, int search_flags = ignore_case | alias_field)
    {
        search_result res;

        for_each([& res, & pattern, search_flags] (contact::contact const & c) {
            contact::contact const * pc = & c;

            if (search_flags & alias_field) {
                search_all(c.alias, pattern, search_flags & ignore_case
                    , [& res, pc] (pfs::unicode::match_item const & m) {
                        res.m.emplace_back(match_item{pc->contact_id, alias_field, m});
                    });
            }

            if (search_flags & desc_field) {
                search_all(c.description, pattern, search_flags & ignore_case
                    , [& res, pc] (pfs::unicode::match_item const & m) {
                        res.m.emplace_back(match_item{pc->contact_id, desc_field, m});
                    });
            }
        });

        return res;
    }

public:
    template <typename ...Args>
    static contact_list make (Args &&... args)
    {
        return contact_list{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
