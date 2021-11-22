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
#include "pfs/uuid.hpp"
#include "pfs/net/chat/contact.hpp"
#include "pfs/net/chat/time_point.hpp"
#include "pfs/net/chat/persistent_storage/sqlite3/contact_list.hpp"

#if PFS_HAVE_STD_FILESYSTEM
namespace filesystem = std::filesystem;
#else
namespace filesystem = pfs::filesystem;
#endif

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

TEST_CASE("contact_list") {
    using contact_t = pfs::net::chat::contact;
    using contact_list_t = pfs::net::chat::sqlite3_ns::contact_list;

    auto contact_list_path = filesystem::temp_directory_path() / "contact_list.db";

    contact_list_t contact_list;
    contact_list.failure.connect([] (std::string const & errstr) {
        fmt::print(stderr, "ERROR: {}\n", errstr);
    });

    REQUIRE(contact_list.open(contact_list_path));

    contact_list.wipe();
    contact_list.begin_transaction();

    for (auto name: NAMES) {
        contact_t c;
        c.id = pfs::generate_uuid();
        c.name = name;
        c.last_activity = pfs::net::chat::current_time_point();

        REQUIRE(contact_list.save(c));
    }

    contact_list.end_transaction();
    contact_list.close();
}

