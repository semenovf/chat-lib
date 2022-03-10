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
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
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
using contact_manager_t = chat::contact_manager<chat::backend::sqlite3::contact_manager>;
using contact_list_t = contact_manager_t::contact_list_type;

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
auto my_uuid = pfs::generate_uuid();
auto my_alias = std::string{"My Alias"};

TEST_CASE("constructors") {
    // Contact list public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<contact_list_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<contact_list_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<contact_list_t>::value);
    REQUIRE(std::is_move_constructible<contact_list_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<contact_list_t>::value);
    REQUIRE(std::is_destructible<contact_list_t>::value);

    // Contact manager public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<contact_manager_t>::value);
    REQUIRE(std::is_move_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<contact_manager_t>::value);
    REQUIRE(std::is_destructible<contact_manager_t>::value);
}

TEST_CASE("initialization") {
    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    chat::error err;
    auto contact_manager = contact_manager_t::make(chat::contact::person{my_uuid, my_alias}
        , dbh, & err);

    REQUIRE(contact_manager);

    if (contact_manager.count() > 0)
        REQUIRE(contact_manager.wipe());

    REQUIRE_EQ(contact_manager.count(), 0);
}

TEST_CASE("contacts") {
    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    auto contact_manager = contact_manager_t::make(
        chat::contact::person{my_uuid, my_alias}, dbh);

    REQUIRE(contact_manager);

    REQUIRE_EQ(contact_manager.add(forward_iterator{NAMES}
            , forward_iterator{NAMES + sizeof(NAMES)/sizeof(NAMES[0])})
        , sizeof(NAMES) / sizeof(NAMES[0]));

    {
        auto count = contact_manager.count();
        REQUIRE_EQ(count, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    std::vector<chat::contact::contact> all_contacts;

    contact_manager.for_each([& all_contacts] (contact_t const & c) {
//         fmt::print("{} | {:10} | {}\n"
//             , to_string(c.id)
//             , c.alias
//             , to_string(c.type));
        all_contacts.push_back(c);
    });

    // No new contacts added as they already exist.
    {
        auto count = contact_manager.add(all_contacts.cbegin(), all_contacts.cend());
        REQUIRE_EQ(count, 0);
    }

    // Change alias for first contact (update)
    {
        auto c = all_contacts[0];
        c.alias = "NewAlias";

        auto count = contact_manager.update(c);
        REQUIRE_EQ(count, 1);
    }

    // Contact was not updated - contact not found
    {
        contact_t c;
        c.id = pfs::generate_uuid();
        c.alias = "Noname";
        c.type = chat::contact::type_enum::person;

        auto count = contact_manager.update(c);
        REQUIRE_EQ(count, 0);
    }

    // Get contact by id
    {
        chat::error err;
        auto c = contact_manager.get(all_contacts[1].id, & err);
        REQUIRE_FALSE(err);
        REQUIRE_EQ(c.alias, all_contacts[1].alias);
        REQUIRE_EQ(c.type, all_contacts[1].type);
    }

    // Get contact by offset
    {
        chat::error err;
        auto c = contact_manager.get(1, & err);
        REQUIRE_FALSE(err);
        REQUIRE_EQ(c.alias, all_contacts[1].alias);
        REQUIRE_EQ(c.type, all_contacts[1].type);
    }

    // Attempt to get non-existent contact
    {
        chat::error err;
        auto c = contact_manager.get(pfs::generate_uuid(), & err);
        REQUIRE(err);
    }
}

TEST_CASE("groups") {
    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    auto contact_manager = contact_manager_t::make(
        chat::contact::person{my_uuid, my_alias}, dbh);

    REQUIRE(contact_manager);

    {
        group_t g;
        g.alias = "Group 0";
        g.id = pfs::generate_uuid();

        REQUIRE_EQ(contact_manager.add(g), 1);

        // No new group added as it already exist
        REQUIRE_EQ(contact_manager.add(g), 0);

        REQUIRE_EQ(contact_manager.group_count(), 1);
    }

    chat::contact::contact_id sample_id;
    std::string sample_alias = "Group 2";

    {
        group_t g;
        g.alias = "Group 1";
        g.id = pfs::generate_uuid();

        REQUIRE_EQ(contact_manager.add(g), 1);

        g.alias = sample_alias;

        REQUIRE_EQ(contact_manager.update(g), 1);
        REQUIRE_EQ(contact_manager.group_count(), 2);

        sample_id = g.id;
    }

    // Group was not updated - group not found
    {
        group_t g;
        g.id = pfs::generate_uuid();
        g.alias = "Noname";

        REQUIRE_EQ(contact_manager.update(g), 0);
    }

    // Get contact by id
    {
        chat::error err;
        auto g = contact_manager.get(sample_id, & err);
        REQUIRE_FALSE(err);
        REQUIRE_EQ(g.alias, sample_alias);
    }

    // Attempt to get non-existent contact
    {
        chat::error err;
        auto g = contact_manager.get(pfs::generate_uuid(), & err);
        REQUIRE(err);
    }

    // Add memebers
    {
        group_t g;
        g.alias = "Group 3";
        g.id = pfs::generate_uuid();

        REQUIRE_EQ(contact_manager.add(g), 1);
        REQUIRE_EQ(contact_manager.group_count(), 3);

        contact_t c1;
        c1.id = pfs::generate_uuid();
        c1.alias = "Contact 1 for " + g.alias;
        c1.type = chat::contact::type_enum::person;

        REQUIRE_EQ(contact_manager.add(c1), 1);

        contact_t c2;
        c2.id = pfs::generate_uuid();
        c2.alias = "Contact 2 for " + g.alias;
        c2.type = chat::contact::type_enum::person;

        REQUIRE_EQ(contact_manager.add(c2), 1);

        // Only person can be added to group now.
        // NOTE This behavior is subject to change in the future.
        contact_t c3;
        c3.id = pfs::generate_uuid();
        c3.alias = "Contact 3 for " + g.alias;
        c3.type = chat::contact::type_enum::group;

        REQUIRE_EQ(contact_manager.add(c3), 1);

        REQUIRE(contact_manager.gref(g.id).add_member(c1.id));
        REQUIRE(contact_manager.gref(g.id).add_member(c2.id));

        chat::error err;
        REQUIRE_FALSE(contact_manager.gref(g.id).add_member(c3.id, & err));

        auto memebers = contact_manager.gref(g.id).members();
        REQUIRE(memebers.size() == 2);

        REQUIRE_EQ(memebers[0].alias, c1.alias);
        REQUIRE_EQ(memebers[1].alias, c2.alias);

        REQUIRE(contact_manager.gref(g.id).is_member_of(c1.id));
        REQUIRE(contact_manager.gref(g.id).is_member_of(c2.id));
        REQUIRE_FALSE(contact_manager.gref(g.id).is_member_of(c3.id));
    }

     // TODO Check remove methods
}
