////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.19 Initial version.
//      2024.11.25 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/contact_list.hpp"
#include "pfs/chat/in_memory.hpp"
#include <cstdint>
#include <map>
#include <vector>

CHAT__NAMESPACE_BEGIN

namespace storage {

class in_memory::contact_list
{
public:
    std::vector<contact::contact> data;
    std::map<contact::id, std::size_t> map;

public:
    contact_list () {}
};

} // namespace storage

using contact_list_t = contact_list<storage::in_memory>;

template <>
contact_list_t::contact_list ()
    : _d(new rep)
{}

template <>
contact_list_t::contact_list (rep * d) noexcept
    : _d(d)
{}

template <> contact_list_t::contact_list (contact_list && other) noexcept = default;
template <> contact_list_t & contact_list_t::operator = (contact_list && other) noexcept = default;
template <> contact_list_t::~contact_list () = default;

template <>
bool contact_list_t::add (contact::contact && c)
{
    _d->map.emplace(c.contact_id, _d->data.size());
    _d->data.push_back(std::move(c));

    return true;
}

template <>
std::size_t contact_list_t::count () const
{
    return _d->data.size();
}

template <>
std::size_t contact_list_t::count (chat_enum type) const
{
    std::size_t count = 0;

    for (auto const & c: _d->data) {
        if (c.type == type)
            count++;
    }

    return count;
}

template <>
contact::contact contact_list_t::get (contact::id id) const
{
    auto pos = _d->map.find(id);

    if (pos != _d->map.end())
        return _d->data[pos->second];

    return contact::contact{};
}

template <>
contact::contact contact_list_t::at (int index) const
{
    if (index >= 0 && index < _d->data.size())
        return _d->data[index];

    return contact::contact{};
}

template <>
void contact_list_t::for_each (std::function<void(contact::contact const &)> f) const
{
    for (auto const & c: _d->data)
        f(c);
}

template <>
void contact_list_t::for_each_until (std::function<bool(contact::contact const &)> f) const
{
    for (auto const & c: _d->data) {
        if (!f(c))
            break;
    }
}

CHAT__NAMESPACE_END
