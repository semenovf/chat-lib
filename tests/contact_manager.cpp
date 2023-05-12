////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2023.04.19 Added `contact_list` test case.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/iterator.hpp"
#include "pfs/memory.hpp"
#include "pfs/universal_id.hpp"
#include "pfs/string_view.hpp"
#include "pfs/time_point.hpp"
#include "pfs/unicode/char.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include <type_traits>
#include <algorithm>

#if PFS__ICU_ENABLED
#   include <unicode/uchar.h>
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

#if PFS__ICU_ENABLED
    // For Uncode ignore case test (using ICU)
    , "Анна"    , "анна"    , "аННа"    , "АННА"    , "АННа"
#endif
};

using contact_t = chat::contact::contact;
using person_t = chat::contact::person;
using group_t = chat::contact::group;
using contact_manager_t = chat::contact_manager<chat::backend::sqlite3::contact_manager>;

struct forward_iterator : public pfs::iterator_facade<
          pfs::forward_iterator_tag
        , forward_iterator
        , person_t, char const **, person_t>
{
    char const ** _p;
    forward_iterator (char const ** p) : _p(p) {}

    reference ref ()
    {
        person_t c;
        c.contact_id = pfs::generate_uuid();
        c.alias = *_p;
        return c;
    }

    pointer ptr () { return _p; }
    void increment (difference_type) { ++_p; }
    bool equals (forward_iterator const & rhs) const { return _p == rhs._p;}
};

auto contact_db_path = pfs::filesystem::temp_directory_path() / "contact.db";
auto my_uuid = pfs::generate_uuid();
auto my_alias = std::string{"My Alias"};

REGISTER_EXCEPTION_TRANSLATOR (chat::error & ex)
{
    static std::string __s {};
    __s = ex.what();
    return doctest::String(__s.c_str(), static_cast<unsigned int>(__s.size()));
}

TEST_CASE("constructors") {
    // Contact list public constructors/assign operators
    using in_memory_contact_list_t = chat::contact_list<chat::backend::in_memory::contact_list>;
    REQUIRE_FALSE(std::is_default_constructible<in_memory_contact_list_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<in_memory_contact_list_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<in_memory_contact_list_t>::value);
    REQUIRE(std::is_move_constructible<in_memory_contact_list_t>::value);
    REQUIRE(std::is_move_assignable<in_memory_contact_list_t>::value);
    REQUIRE(std::is_destructible<in_memory_contact_list_t>::value);

    // Contact manager public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<contact_manager_t>::value);
    REQUIRE(std::is_move_constructible<contact_manager_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<contact_manager_t>::value);
    REQUIRE(std::is_destructible<contact_manager_t>::value);
}

TEST_CASE("initialization") {

    if (pfs::filesystem::exists(contact_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(contact_db_path) > 0);
    }

    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    auto contact_manager = contact_manager_t::make(
        chat::contact::person{my_uuid, my_alias}, dbh);

    REQUIRE(contact_manager);

    if (contact_manager.count() > 0)
        contact_manager.wipe();

    REQUIRE_EQ(contact_manager.count(), 0);

    // Populate new data
    auto batch_add = [& contact_manager] {
        forward_iterator first{NAMES};
        forward_iterator last{NAMES + sizeof(NAMES)/sizeof(NAMES[0])};

        while (first != last) {
            contact_manager.add(*first);
            ++first;
        }

        return true;
    };

    REQUIRE(contact_manager.transaction(batch_add));
}

// Reside this tests before any modifications of database
TEST_CASE("contact_list") {
    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    auto contact_manager = contact_manager_t::make(
        chat::contact::person{my_uuid, my_alias}, dbh);

    REQUIRE(contact_manager);

    {
        auto contact_list = contact_manager.contacts([] (contact_t const & c)->bool {
            auto predicate = [] (pfs::unicode::char_t a, pfs::unicode::char_t b)->bool {
                return a == b;
            };
            pfs::string_view needle {"ile"};
            return std::search(c.alias.begin(), c.alias.end(), needle.begin()
                , needle.end(), predicate) != c.alias.end();
        });

        // Kile and Mile
        REQUIRE_EQ(contact_list.count(), 2);
        CHECK((contact_list.at(0).alias == "Kile" || contact_list.at(0).alias == "Mile"));
        CHECK((contact_list.at(1).alias == "Kile" || contact_list.at(1).alias == "Mile"));

        //contact_list.for_each([] (contact_t const & c) {
        //    fmt::print("{} | {:10} | {}\n", c.contact_id, c.alias, to_string(c.type));
        //});
    }

    {
        // Search with ignore case using standard C or ICU features

        auto contact_list = contact_manager.contacts([] (contact_t const & c)->bool {
            auto predicate = [] (pfs::unicode::char_t a, pfs::unicode::char_t b)->bool {
                return pfs::unicode::to_lower(a) == pfs::unicode::to_lower(b);
            };

            pfs::string_view needle {"en"};
            return std::search(c.alias.begin(), c.alias.end(), needle.begin()
                , needle.end(), predicate) != c.alias.end();
        });

        // Gwenore, Alden, Gene, Jaquenetta, Enrika, Ken, Laurene
        CHECK_EQ(contact_list.count(), 7);

        contact_list.for_each([] (contact_t const & c) {
            fmt::print("{} | {:10} | {}\n", c.contact_id, c.alias, to_string(c.type));
        });
    }

#if PFS__ICU_ENABLED
        auto contact_list = contact_manager.contacts([] (contact_t const & c)->bool {
            auto predicate = [] (pfs::unicode::char_t a, pfs::unicode::char_t b)->bool {
                return pfs::unicode::to_lower(a) == pfs::unicode::to_lower(b);
            };

            pfs::string_view needle {"АнНа"};
            return std::search(c.alias.begin(), c.alias.end(), needle.begin()
                , needle.end(), predicate) != c.alias.end();
        });

        CHECK_EQ(contact_list.count(), 5);

        contact_list.for_each([] (contact_t const & c) {
            fmt::print("{} | {:10} | {}\n", c.contact_id, c.alias, to_string(c.type));
        });
#endif
}

template <typename ContactListBackend>
void test_contacts ()
{
    auto dbh = chat::backend::sqlite3::make_handle(contact_db_path, true);

    REQUIRE(dbh);

    auto contact_manager = contact_manager_t::make(
        chat::contact::person{my_uuid, my_alias}, dbh);

    REQUIRE(contact_manager);

    {
        auto count = contact_manager.count();
        REQUIRE_EQ(count, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    std::vector<person_t> all_contacts;

    auto contacts = contact_manager.contacts<chat::contact_list<ContactListBackend>>();

    contacts.for_each([& all_contacts] (contact_t const & c) {
        //fmt::print("{} | {:10} | {}\n", c.contact_id, c.alias, to_string(c.type));
        all_contacts.push_back(person_t {c.contact_id, c.alias, c.avatar, c.description});
    });

    // No new contacts added as they already exist.
    {
        std::size_t count = 0;
        auto batch_add = [& contact_manager, & all_contacts, & count] {
            bool success = true;
            auto first = all_contacts.cbegin();
            auto last = all_contacts.cend();

            for (; success && first != last; ++first) {
                if (contact_manager.add(*first))
                    count++;
            }

            return success;
        };

        REQUIRE(contact_manager.transaction(batch_add));
        REQUIRE_EQ(count, 0);
    }

    // Change alias for first contact (update)
    {
        auto c = all_contacts[0];
        c.alias = "NewAlias";

        auto success = contact_manager.update(c);
        REQUIRE(success);
    }

    // Contact was not updated - contact not found
    {
        person_t c;
        c.contact_id = pfs::generate_uuid();
        c.alias = "Noname";

        auto success = contact_manager.update(c);
        REQUIRE_FALSE(success);
    }

    // Get contact by id
    {
        auto c = contact_manager.get(all_contacts[1].contact_id);
        REQUIRE_EQ(c.alias, all_contacts[1].alias);
        REQUIRE_EQ(c.type, chat::conversation_enum::person);
    }

    // Get contact by offset
    {
        auto c = contact_manager.at(1);
        REQUIRE_EQ(c.alias, all_contacts[1].alias);
        REQUIRE_EQ(c.type, chat::conversation_enum::person);
    }

    // Attempt to get non-existent contact
    {
        auto c = contact_manager.get(pfs::generate_uuid());
        REQUIRE_FALSE(is_valid(c));
    }
}

TEST_CASE("contacts") {
    test_contacts<chat::backend::in_memory::contact_list>();
    test_contacts<chat::backend::sqlite3::contact_list>();
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
        g.contact_id = pfs::generate_uuid();
        g.creator_id = my_uuid;

        REQUIRE(contact_manager.add(g));

        // No new group added as it already exist
        REQUIRE_FALSE(contact_manager.gref(g.contact_id).add_member(my_uuid));

        REQUIRE_EQ(contact_manager.groups_count(), 1);
    }

    chat::contact::id sample_id;
    std::string sample_alias = "Group 2";

    {
        group_t g;
        g.alias = "Group 1";
        g.contact_id = pfs::generate_uuid();
        g.creator_id = my_uuid;

        REQUIRE(contact_manager.add(g));

        g.alias = sample_alias;

        REQUIRE(contact_manager.update(g));
        REQUIRE_EQ(contact_manager.groups_count(), 2);

        sample_id = g.contact_id;
    }

    // Group was not updated - group not found
    {
        group_t g;
        g.contact_id = pfs::generate_uuid();
        g.alias = "Noname";

        REQUIRE_FALSE(contact_manager.update(g));
    }

    // Get contact by id
    {
        auto g = contact_manager.get(sample_id);
        REQUIRE_EQ(g.alias, sample_alias);
    }

    // Attempt to get non-existent contact
    {
        auto g = contact_manager.get(pfs::generate_uuid());
        REQUIRE_FALSE(is_valid(g));
    }

    // Add memebers
    {
        group_t g;
        g.alias = "Group 3";
        g.contact_id = pfs::generate_uuid();
        g.creator_id = my_uuid;

        REQUIRE(contact_manager.add(g));

        REQUIRE_EQ(contact_manager.groups_count(), 3);

        person_t c1;
        c1.contact_id = pfs::generate_uuid();
        c1.alias = "Contact 1 for " + g.alias;

        REQUIRE(contact_manager.add(c1));

        person_t c2;
        c2.contact_id = pfs::generate_uuid();
        c2.alias = "Contact 2 for " + g.alias;

        REQUIRE(contact_manager.add(c2));

        // Only person can be added to group now.
        // NOTE This behavior is subject to change in the future.
        group_t c3;
        c3.contact_id = pfs::generate_uuid();
        c3.alias = "Contact 3 for " + g.alias;
        c3.creator_id = my_uuid;
        REQUIRE(contact_manager.add(c3));

        REQUIRE(contact_manager.gref(g.contact_id).add_member(c1.contact_id));
        REQUIRE(contact_manager.gref(g.contact_id).add_member(c2.contact_id));

        REQUIRE_THROWS(contact_manager.gref(g.contact_id).add_member(c3.contact_id));

        auto memebers = contact_manager.gref(g.contact_id).members();
        REQUIRE(memebers.size() == 3);

        REQUIRE_EQ(memebers[0].alias, my_alias);
        REQUIRE_EQ(memebers[1].alias, c1.alias);
        REQUIRE_EQ(memebers[2].alias, c2.alias);

        REQUIRE(contact_manager.gref(g.contact_id).is_member_of(my_uuid));
        REQUIRE(contact_manager.gref(g.contact_id).is_member_of(c1.contact_id));
        REQUIRE(contact_manager.gref(g.contact_id).is_member_of(c2.contact_id));
        REQUIRE_FALSE(contact_manager.gref(g.contact_id).is_member_of(c3.contact_id));

        // Including my contact
        REQUIRE_EQ(contact_manager.gref(g.contact_id).count(), 3);
    }

    // TODO Check remove methods
}
