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
#include "pfs/unicode/utf8_iterator.hpp"
#include <algorithm>
#include <functional>
#include <vector>

#include "pfs/log.hpp"

namespace chat {

template <typename Backend>
class contact_list final
{
    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    contact_list () = default;

public:
    contact_list (rep_type && rep);
    contact_list (contact_list const & other) = delete;
    contact_list (contact_list && other) = default;
    contact_list & operator = (contact_list const & other) = delete;
    contact_list & operator = (contact_list && other) = delete;
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

private:
    ////////////////////////////////////////////////////////////////////////////
    // Search specific types and methods.
    ////////////////////////////////////////////////////////////////////////////
    enum search_flag
    {
          ignore_case = 1 << 0
        , alias_field = 1 << 1
        , desc_field  = 1 << 2
    };

    enum class field_enum { alias, desc };

    struct match_item
    {
        int index;        // contact index in contact list (clist)
        field_enum field; // field that matches search pattern
        int first;        // first position of the matched subrange
        int last;         // last position of the matched subrange (exclusive)
    };

    struct match
    {
        contact_list clist;
        std::vector<match_item> m;
    };

    static void scan (std::string const & s, std::string const & pattern
        , int search_flags, std::function<void()> && f)
    {
        using utf8_input_iterator = pfs::unicode::utf8_input_iterator<std::string::const_iterator>;

        auto p_first = utf8_input_iterator::begin(pattern.begin(), pattern.end());
        auto p_last  = p_first.end();
        auto p_len   = std::distance(p_first, p_last);
        auto first   = utf8_input_iterator::begin(s.begin(), s.end());
        auto last    = first.end();

        auto predicate = (search_flags & search_flag::ignore_case)
            ? [] (pfs::unicode::char_t a, pfs::unicode::char_t b)->bool { return pfs::unicode::to_lower(a) == pfs::unicode::to_lower(b); }
            : [] (pfs::unicode::char_t a, pfs::unicode::char_t b)->bool { return a == b; };

        auto pos = std::search(first, last, p_first, p_last, predicate);

        while (pos != last) {
            std::string prefix {first.base(), pos.base()};
            std::string substr {pos.base(), pos.base() + pattern.size()};

//                     m.m.emplace_back();
//                     auto mi = m.m.back();
//
//                     m.index = ?;        // contact index in contact list (clist)
//                     mi.field = alias_field;
//                     mi.first = ;        // first position of the matched subrange
//                     mi.last  ;         // last position of the matched subrange (inclusive)

            std::advance(pos, p_len);
            std::string suffix {pos.base(), last.base()};

            LOGD("", "{}[{}]{}", prefix, substr, suffix);

            pos = std::search(pos, last, p_first, p_last, predicate);
        }
    }

public:
    /**
     * Searches contact list for specified @a pattern.
     */
    match search (std::string const & pattern, int search_flags = ignore_case | alias_field)
    {
        match res;

        for_each([& res, & pattern, search_flags] (contact::contact const & c) {
            if (search_flags & alias_field) {
                scan(c.alias, pattern, search_flags, [] {

                });
            }

            if (search_flags & desc_field) {
                scan(c.description, pattern, search_flags, [] {

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
