////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/iterator.hpp"
#include "pfs/uuid.hpp"
#include "pfs/time_point.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/persistent_storage/sqlite3/contact_manager.hpp"
#include <type_traits>

char const * NAMES[] = {
      "Laurene"  , "Fred"   , "Rosita"  , "Valdemar", "Shaylyn"
    , "Maribelle", "Gwenore", "Willow"  , "Linda"   , "Bobbette"
    , "Kane"     , "Ricki"  , "Gun"     , "Laetitia", "Jaquenetta"
    , "Gray"     , "Stepha" , "Emili"   , "Gerrard" , "Elroy"
    , "Augusto"  , "Tate"   , "Bryana"  , "Moira"   , "Adrian"
    , "Thomasa"  , "Kile"   , "Martino" , "Rolf"    , "Emylee"
    , "Hercule"  , "Mile"   , "Boyce"   , "Lurette" , "Allyson"
    , "Imelda"   , "Gal"    , "Vikky"   , "Dody"    , "Cindee"
    , "Merrili"  , "Esteban", "Janet"   , "Tirrell" , "Malanie"
    , "Ester"    , "Wilbur" , "Mike"    , "Alden"   , "Gerri"
    , "Nicoline" , "Rozalie", "Patrizia", "Ursala"  , "Gene"
    , "Ancell"   , "Roxi"   , "Tamqrah" , "Billy"   , "Kitty"
    , "Rosette"  , "Gardy"  , "Bianca"  , "Amandie" , "Hew"
    , "Shelby"   , "Enrika" , "Emelia"  , "Ken"     , "Lotti"
    , "Cherey"   , "Efrem"  , "Eb"      , "Ezechiel", "Melody"
    , "Blane"    , "Fifi"   , "Graehme" , "Arnoldo" , "Brigit"
    , "Randee"   , "Bogart" , "Parke"   , "Ashla"   , "Wash"
    , "Karisa"   , "Trey"   , "Lorry"   , "Danielle", "Delly"
    , "Codie"    , "Timmy"  , "Velma"   , "Glynda"  , "Amara"
    , "Garey"    , "Mirabel", "Eliot"   , "Mata"    , "Flemming"
};

using contact_t = chat::contact::contact;
using group_t = chat::contact::group;
using contact_manager_t = chat::persistent_storage::sqlite3::contact_manager;
using contact_list_t = chat::persistent_storage::sqlite3::contact_list;

struct forward_iterator : public pfs::iterator_facade<
          pfs::forward_iterator_tag
        , forward_iterator
        , contact_t, char const **, contact_t>
{
    char const ** _p;
    forward_iterator (char const ** p) : _p(p) {}

    reference ref ()
    {
        contact_t c;
        c.id = pfs::generate_uuid();
        c.alias = *_p;
        c.type = chat::contact::type_enum::person;
        return c;
    }

    pointer ptr () { return _p; }
    void increment (difference_type) { ++_p; }
    bool equals (forward_iterator const & rhs) const { return _p == rhs._p;}
};

auto on_failure = [] (std::string const & errstr) {
    fmt::print(stderr, "ERROR: {}\n", errstr);
};

auto contact_db_path = pfs::filesystem::temp_directory_path() / "contact.db";

TEST_CASE("constructors") {
    // Contact list public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<contact_list_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<contact_list_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<contact_list_t>::value);
    REQUIRE_FALSE(std::is_move_constructible<contact_list_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<contact_list_t>::value);
    REQUIRE_FALSE(std::is_destructible<contact_list_t>::value);

    // Contact manager public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_move_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<contact_manager_t>::value);
    REQUIRE(std::is_destructible<contact_manager_t>::value);
}

TEST_CASE("initialization") {
    auto dbh = chat::persistent_storage::sqlite3::make_handle(contact_db_path
        , true, on_failure);

    REQUIRE(dbh);

    contact_manager_t contact_manager{dbh, on_failure};

    REQUIRE(contact_manager);

    auto & contacts = contact_manager.contacts();

    if (contacts.count() > 0)
        REQUIRE(contact_manager.wipe());

    REQUIRE_EQ(contacts.count(), 0);
}

TEST_CASE("contacts") {
    auto dbh = chat::persistent_storage::sqlite3::make_handle(contact_db_path
        , true, on_failure);

    REQUIRE(dbh);

    contact_manager_t contact_manager{dbh, on_failure};

    REQUIRE(contact_manager);

    auto & contacts = contact_manager.contacts();

    REQUIRE_EQ(contacts.add(forward_iterator{NAMES}
            , forward_iterator{NAMES + sizeof(NAMES)/sizeof(NAMES[0])})
        , sizeof(NAMES) / sizeof(NAMES[0]));

    {
        auto count = contacts.count();
        REQUIRE_EQ(count, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    std::vector<chat::contact::contact> all_contacts;

    contacts.all_of([& all_contacts] (contact_t const & c) {
//         fmt::print("{} | {:10} | {}\n"
//             , to_string(c.id)
//             , c.alias
//             , to_string(c.type));
        all_contacts.push_back(c);
    });

    // No new contacts added as they already exist.
    {
        auto count = contacts.add(all_contacts.cbegin(), all_contacts.cend());
        REQUIRE_EQ(count, 0);
    }

    // Change alias for first contact (update)
    {
        auto c = all_contacts[0];
        c.alias = "NewAlias";

        auto count = contacts.update(c);
        REQUIRE_EQ(count, 1);
    }

    // Contact was not updated - contact not found
    {
        contact_t c;
        c.id = pfs::generate_uuid();
        c.alias = "Noname";
        c.type = chat::contact::type_enum::person;

        auto count = contacts.update(c);
        REQUIRE_EQ(count, 0);
    }

    // Get contact by id
    {
        auto c = contacts.get(all_contacts[1].id);
        REQUIRE(c);
        REQUIRE_EQ(c->alias, all_contacts[1].alias);
        REQUIRE_EQ(c->type, all_contacts[1].type);
    }

    // Get contact by offset
    {
        auto c = contacts.get(1);
        REQUIRE(c);
        REQUIRE_EQ(c->alias, all_contacts[1].alias);
        REQUIRE_EQ(c->type, all_contacts[1].type);
    }

    // Attempt to get non-existent contact
    {
        auto c = contacts.get(pfs::generate_uuid());
        REQUIRE(!c);
    }
}


TEST_CASE("groups") {
    auto dbh = chat::persistent_storage::sqlite3::make_handle(contact_db_path
        , true, on_failure);

    REQUIRE(dbh);

    contact_manager_t contact_manager{dbh, on_failure};

    REQUIRE(contact_manager);

    auto & contacts = contact_manager.contacts();
    auto & groups = contact_manager.groups();

    {
        group_t g;
        g.alias = "Group 0";
        g.id = pfs::generate_uuid();

        REQUIRE_EQ(groups.add(g), 1);

        // No new group added as it already exist
        REQUIRE_EQ(groups.add(g), 0);

        REQUIRE_EQ(groups.count(), 1);
    }

    chat::contact::contact_id sample_id;
    std::string sample_alias = "Group 2";

    {
        group_t g;
        g.alias = "Group 1";
        g.id = pfs::generate_uuid();

        REQUIRE_EQ(groups.add(g), 1);

        g.alias = sample_alias;

        REQUIRE_EQ(groups.update(g), 1);

        REQUIRE_EQ(groups.count(), 2);

        sample_id = g.id;
    }

    // Group was not updated - group not found
    {
        group_t g;
        g.id = pfs::generate_uuid();
        g.alias = "Noname";

        REQUIRE_EQ(groups.update(g), 0);
    }

    // Get contact by id
    {
        auto g = groups.get(sample_id);
        REQUIRE(g.has_value());
        REQUIRE(g);
        REQUIRE_EQ(g->alias, sample_alias);
    }

    // Attempt to get non-existent contact
    {
        auto g = groups.get(pfs::generate_uuid());
        REQUIRE(!g);
    }

    // Add memebers
    {
        group_t g;
        g.alias = "Group 3";
        g.id = pfs::generate_uuid();

        REQUIRE_EQ(groups.add(g), 1);
        REQUIRE_EQ(groups.count(), 3);

        contact_t c1;
        c1.id = pfs::generate_uuid();
        c1.alias = "Contact 1 for " + g.alias;
        c1.type = chat::contact::type_enum::person;

        REQUIRE_EQ(contacts.add(c1), 1);

        contact_t c2;
        c2.id = pfs::generate_uuid();
        c2.alias = "Contact 2 for " + g.alias;
        c2.type = chat::contact::type_enum::person;

        REQUIRE_EQ(contacts.add(c2), 1);

        // Only person can be added to group now.
        // NOTE This behavior is subject to change in the future.
        contact_t c3;
        c3.id = pfs::generate_uuid();
        c3.alias = "Contact 3 for " + g.alias;
        c3.type = chat::contact::type_enum::group;

        REQUIRE_EQ(contacts.add(c3), 1);

        REQUIRE(groups.add_member(g.id, c1.id));
        REQUIRE(groups.add_member(g.id, c2.id));
        REQUIRE_FALSE(groups.add_member(g.id, c3.id));

        auto memebers = groups.members(g.id);
        REQUIRE(memebers.size() == 2);

        REQUIRE_EQ(memebers[0].alias, c1.alias);
        REQUIRE_EQ(memebers[1].alias, c2.alias);
    }
}
