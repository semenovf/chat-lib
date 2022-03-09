////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.24 Initial version.
//      2022.02.17 Refactored to use backend.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include <functional>

namespace chat {

template <typename Backend>
class contact_list final
{
    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

private:
    contact_list () = default;
    contact_list (rep_type && rep);
    contact_list (contact_list const & other) = delete;
    contact_list & operator = (contact_list const & other) = delete;
    contact_list & operator = (contact_list && other) = delete;

public:
    contact_list (contact_list && other) = default;
    ~contact_list () = default;

private:
    /**
     * Batch add series of contacts.
     *
     * @return Total contacts added or -1 on error.
     */
    int batch_add (std::function<bool()> has_next
        , std::function<contact::contact()> next
        , error * perr = nullptr);

public:
    /**
     */
    std::size_t count () const;

    /**
     */
    std::size_t count (contact::type_enum type) const;

    /**
     * Adds contact.
     *
     * @return @c 1 if contact successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::contact const & c, error * perr = nullptr);

    /**
     * Adds series of contacts.
     *
     * @return Total contacts added or -1 on error.
     */
    template <typename ForwardIt>
    int add (ForwardIt first, ForwardIt last, error * perr = nullptr)
    {
        return batch_add([& first, last] {return first != last; }
            , [& first] { return *first++; }
            , perr);
    }

    /**
     * Adds group.
     *
     * @return @c 1 if group successfully added or @c 0 if contact already
     *         exists with @c contact_id or @c -1 on error.
     */
    int add (contact::group const & g, error * perr = nullptr)
    {
        contact::contact c {g.id
            , g.alias
            , g.avatar
            , g.description
            , chat::contact::type_enum::group};
        return add(c, perr);
    }

    /**
     * @return @c 1 if contact successfully updated or @c 0 if contact not found
     *         with @c contact_id or @c -1 on error.
     */
    int update (contact::contact const & c, error * perr = nullptr);

    /**
     * @return @c 1 if group successfully updated or @c 0 if group not found
     *     with @c contact_id or @c -1 on error.
     */
    int update (contact::group const & g, error * perr = nullptr)
    {
        contact::contact c {g.id
            , g.alias
            , g.avatar
            , g.description
            , chat::contact::type_enum::group};
        return update(c, perr);
    }

    /**
     * Get contact by @a id.
     */
    contact::contact get (contact::contact_id id, error * perr = nullptr) const;

    /**
     * Get contact by @a id.
     */
    contact::contact get_or (contact::contact_id id, contact::contact const & default_value) const
    {
        error err;
        auto c = get(id, & err);
        return err ? default_value : c;
    }

    /**
     * Get contact by @a offset.
     */
    contact::contact get (int offset, error * perr = nullptr) const;

    /**
     * Get contact by @a offset.
     */
    contact::contact get_or (int offset, contact::contact const & default_value) const
    {
        error err;
        auto c = get(offset, & err);
        return err ? default_value : c;
    }

    /**
     * Fetch all contacts and process them by @a f
     *
     * @return @c true If no error occured or @c false otherwise.
     */
    bool for_each (std::function<void(contact::contact const &)> f, error * perr = nullptr);

public:
    template <typename ...Args>
    static contact_list make (Args &&... args)
    {
        return contact_list{Backend::make(std::forward<Args>(args)...)};
    }
};

} // namespace chat
