////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.19 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/contact_list.hpp"
#include "pfs/chat/backend/in_memory/contact_list.hpp"

namespace chat {

using BACKEND = backend::in_memory::contact_list;

template <>
contact_list<BACKEND>::contact_list (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
std::size_t
contact_list<BACKEND>::count () const
{
    return _rep.data.size();
}

template <>
std::size_t
contact_list<BACKEND>::count (conversation_enum type) const
{
    std::size_t count = 0;

    for (auto const & c: _rep.data) {
        if (c.type == type)
            count++;
    }

    return count;
}

template <>
contact::contact
contact_list<BACKEND>::get (contact::id id) const
{
    auto pos = _rep.map.find(id);

    if (pos != _rep.map.end())
        return _rep.data[pos->second];

    return contact::contact{};
}

template <>
contact::contact
contact_list<BACKEND>::at (int index) const
{
    if (index >= 0 && index < _rep.data.size())
        return _rep.data[index];

    return contact::contact{};
}

template <>
void
contact_list<BACKEND>::for_each (std::function<void(contact::contact const &)> f) const
{
    for (auto const & c: _rep.data)
        f(c);
}

template <>
void
contact_list<BACKEND>::for_each_until (std::function<bool(contact::contact const &)> f) const
{
    for (auto const & c: _rep.data) {
        if (!f(c))
            break;
    }
}

} // namespace chat
