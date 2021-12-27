////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
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
#include "pfs/chat/persistent_storage/sqlite3/contact_list.hpp"

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
        c.name = *_p;
        c.alias = *_p;
        c.type = chat::contact::type_enum::person;
        c.last_activity = pfs::current_utc_time_point();
        return c;
    }

    pointer ptr () { return _p; }
    void increment (difference_type) { ++_p; }
    bool equals (forward_iterator const & rhs) const { return _p == rhs._p;}
};

TEST_CASE("contact_list") {
    auto on_failure = [] (std::string const & errstr) {
        fmt::print(stderr, "ERROR: {}\n", errstr);
        REQUIRE(false);
    };

    auto contact_list_path = pfs::filesystem::temp_directory_path() / "contact_list.db";

    auto dbh = chat::persistent_storage::sqlite3::make_handle(contact_list_path
        , true, on_failure);

    REQUIRE(dbh);

    contact_list_t contact_list{dbh, on_failure};

    REQUIRE(contact_list);
    REQUIRE(contact_list.wipe());

    REQUIRE_EQ(contact_list.add(forward_iterator{NAMES}
            , forward_iterator{NAMES + sizeof(NAMES)/sizeof(NAMES[0])})
        , sizeof(NAMES) / sizeof(NAMES[0]));

    {
        auto count = contact_list.count();
        REQUIRE_EQ(count, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    std::vector<chat::contact::contact> all_contacts;

    contact_list.all_of([& all_contacts] (contact_t const & c) {
        fmt::print("{} | {:10} | {:10} | {:10} | {}\n"
            , to_string(c.id)
            , c.name
            , c.alias
            , to_string(c.type)
            , to_string(c.last_activity));
        all_contacts.push_back(c);
    });

    // No new contacts added as they already exist.
    {
        auto count = contact_list.add(all_contacts.cbegin(), all_contacts.cend());
        REQUIRE_EQ(count, 0);
    }

    // Change alias for first contact (update)
    {
        auto c = all_contacts[0];
        c.alias = "NewAlias";

        auto count = contact_list.update(c);
        REQUIRE_EQ(count, 1);
    }

    // Contact was not updated - contact not found
    {
        contact_t c;
        c.id = pfs::generate_uuid();
        c.name = "Noname";
        c.alias = "Noname";
        c.type = chat::contact::type_enum::person;
        c.last_activity = pfs::current_utc_time_point();

        auto count = contact_list.update(c);
        REQUIRE_EQ(count, 0);
    }

    // Get contact
    {
        auto c = contact_list.get(all_contacts[1].id);
        REQUIRE(c);
        REQUIRE_EQ(c->name, all_contacts[1].name);
        REQUIRE_EQ(c->alias, all_contacts[1].alias);
        REQUIRE_EQ(c->type, all_contacts[1].type);
    }

    // Attempt to get non-existent contact
    {
        auto c = contact_list.get(pfs::generate_uuid());
        REQUIRE(!c);
    }
}
