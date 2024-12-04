////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.12.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "chat/sqlite3.hpp"
#include "chat/contact.hpp"
#include <cstdint>
#include <map>
#include <vector>
#include <string>

namespace chat {
namespace storage {

class sqlite3::contact_list
{
public:
    struct cache_data
    {
        int offset;
        int limit;
        std::vector<contact::contact> data;
        std::map<contact::id, std::size_t> map;
    };

public:
    relational_database_t * pdb {nullptr};
    mutable cache_data cache;
    std::string table_name;

public:
    contact_list (std::string tname, relational_database_t & db)
        : pdb(& db)
        , table_name(std::move(tname))
    {}

public:
    void prefetch (int offset, int limit);

public: // static
    static void fill_contact (relational_database_t::result_type & result, contact::contact & c);
};

}} // namespace chat::storage


