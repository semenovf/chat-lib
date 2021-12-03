////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
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
#include "pfs/chat/persistent_storage/contact_list.hpp"

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

using contact_t = pfs::chat::contact::contact;
using contact_list_t = pfs::chat::contact::contact_list;

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
        c.type = pfs::chat::contact::type_enum::person;
        c.last_activity = pfs::current_utc_time_point();
        return c;
    }

    pointer ptr () { return _p; }
    void increment (difference_type) { ++_p; }
    bool equals (forward_iterator const & rhs) const { return _p == rhs._p;}
};

TEST_CASE("contact_list") {
    auto contact_list_path = pfs::filesystem::temp_directory_path() / "contact_list.db";

    auto dbh = contact_list_t::make_handle();

    REQUIRE(dbh->open(contact_list_path));

    contact_list_t contact_list {dbh};
    contact_list.failure.connect([] (std::string const & errstr) {
        fmt::print(stderr, "ERROR: {}\n", errstr);
        REQUIRE(false);
    });

    REQUIRE(contact_list.open());

    contact_list.wipe();

    REQUIRE(contact_list.save_range(forward_iterator{NAMES}
        , forward_iterator{NAMES + sizeof(NAMES)/sizeof(NAMES[0])}));

    contact_list.all_of([] (contact_t const & c) {
        fmt::print("{} | {:10} | {:10} | {:10} | {}\n"
            , std::to_string(c.id)
            , c.name
            , c.alias
            , std::to_string(c.type)
            , to_string(c.last_activity));
    });

    contact_list.close();
}
