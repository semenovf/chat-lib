////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.21 Initial version.
//      2023.04.19 Added `contact_list` test case.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/in_memory.hpp"
#include "pfs/chat/sqlite3.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/fmt.hpp>
#include <pfs/iterator.hpp>
#include <pfs/memory.hpp>
#include <pfs/universal_id.hpp>
#include <pfs/string_view.hpp>
#include <pfs/time_point.hpp>
#include <pfs/unicode/char.hpp>
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
using contact_manager_t = chat::contact_manager<chat::storage::sqlite3>;

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
    static std::string static_tmp_str {};
    static_tmp_str = ex.what();
    return doctest::String(static_tmp_str.c_str(), static_cast<unsigned int>(static_tmp_str.size()));
}

TEST_CASE("constructors") {
    // Contact list public constructors/assign operators
    using in_memory_contact_list_t = chat::contact_list<chat::storage::in_memory>;

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
    REQUIRE(std::is_move_assignable<contact_manager_t>::value);
    REQUIRE(std::is_destructible<contact_manager_t>::value);
}

TEST_CASE("initialization") {

    if (pfs::filesystem::exists(contact_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(contact_db_path) > 0);
    }

    auto db = debby::sqlite3::make(contact_db_path);

    REQUIRE(db);

    chat::contact::person my_contact {my_uuid, my_alias};
    auto contact_manager = contact_manager_t::make(my_contact, db);

    REQUIRE(contact_manager);

    if (contact_manager.count() > 0)
        contact_manager.clear();

    REQUIRE_EQ(contact_manager.count(), 0);

    // Populate new data
    auto batch_add = [& contact_manager] {
        forward_iterator first{NAMES};
        forward_iterator last{NAMES + sizeof(NAMES)/sizeof(NAMES[0])};

        while (first != last) {
            contact_manager.add(*first);
            ++first;
        }

        return pfs::optional<std::string>{};
    };

    REQUIRE_EQ(contact_manager.transaction(batch_add), pfs::nullopt);
}

// Reside this tests before any modifications of database
TEST_CASE("contact_list") {
    auto db = debby::sqlite3::make(contact_db_path);

    REQUIRE(db);

    auto contact_manager = contact_manager_t::make(db);

    REQUIRE(contact_manager);

    {
        auto contact_list = contact_manager.contacts([] (contact_t const & c) -> bool {
            auto predicate = [] (pfs::unicode::char_t a, pfs::unicode::char_t b) -> bool {
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

        fmt::println("\n-- Kile and Mile ----");
        contact_list.for_each([] (contact_t const & c) {
            fmt::print("{} | {:10} | {}\n", c.contact_id, c.alias, to_string(c.type));
        });
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

        fmt::println("\n-- Search result with ignore case using standard C ----");
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

        fmt::println("\n-- Search result with ignore case using ICU ----");
        contact_list.for_each([] (contact_t const & c) {
            fmt::print("{} | {:10} | {}\n", c.contact_id, c.alias, to_string(c.type));
        });
#endif
}

template <typename Storage>
void test_contacts ()
{
    auto db = debby::sqlite3::make(contact_db_path);

    REQUIRE(db);

    auto contact_manager = contact_manager_t::make(db);

    REQUIRE(contact_manager);

    {
        auto count = contact_manager.count();
        REQUIRE_EQ(count, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    std::vector<person_t> all_contacts;

    auto contacts = contact_manager.contacts<chat::contact_list<Storage>>();

    contacts.for_each([& all_contacts] (contact_t const & c) {
        //fmt::print("{} | {:10} | {}\n", c.contact_id, c.alias, to_string(c.type));
        all_contacts.push_back(person_t {c.contact_id, c.alias, c.avatar, c.description, c.extra});
    });

    // No new contacts added as they already exist.
    {
        std::size_t count = 0;
        auto batch_add = [& contact_manager, & all_contacts, & count] {
            bool success = true;
            auto first = all_contacts.cbegin();
            auto last = all_contacts.cend();

            for (; success && first != last; ++first) {
                if (contact_manager.add(person_t{*first}))
                    count++;
            }

            return pfs::optional<std::string>{};
        };

        REQUIRE(!contact_manager.transaction(batch_add));
        REQUIRE_EQ(count, 0);
    }

    // Change alias for the first contact (update)
    {
        auto c = all_contacts[0];
        c.alias = "NewAlias";

        auto success = contact_manager.update(std::move(c));
        REQUIRE(success);
    }

    // Contact was not updated - contact not found
    {
        person_t c;
        c.contact_id = pfs::generate_uuid();
        c.alias = "Noname";

        auto success = contact_manager.update(std::move(c));
        REQUIRE_FALSE(success);
    }

    // Get contact by id
    {
        auto c = contact_manager.get(all_contacts[1].contact_id);
        REQUIRE_EQ(c.alias, all_contacts[1].alias);
        REQUIRE_EQ(c.type, chat::chat_enum::person);
    }

    // Get contact by offset
    {
        auto c = contact_manager.at(1);
        REQUIRE_EQ(c.alias, all_contacts[1].alias);
        REQUIRE_EQ(c.type, chat::chat_enum::person);
    }

    // Attempt to get non-existent contact
    {
        auto c = contact_manager.get(pfs::generate_uuid());
        REQUIRE_FALSE(is_valid(c));
    }
}

TEST_CASE("contacts") {
    test_contacts<chat::storage::in_memory>();
    test_contacts<chat::storage::sqlite3>();
}

TEST_CASE("groups") {
    auto db = debby::sqlite3::make(contact_db_path);

    REQUIRE(db);

    auto contact_manager = contact_manager_t::make(db);

    REQUIRE(contact_manager);

    {
        group_t g;
        g.alias = "Group 0";
        g.contact_id = pfs::generate_uuid();
        g.creator_id = my_uuid;

        REQUIRE(contact_manager.add(std::move(g)));

        // No new group added as it already exist
        REQUIRE_FALSE(contact_manager.gref(g.contact_id).add_member(my_uuid));

        REQUIRE_EQ(contact_manager.group_count(), 1);
    }

    chat::contact::id sample_id;
    std::string sample_alias = "Group 2";

    {
        group_t g;
        g.alias = "Group 1";
        g.contact_id = pfs::generate_uuid();
        g.creator_id = my_uuid;

        REQUIRE(contact_manager.add(group_t{g}));

        g.alias = sample_alias;

        REQUIRE(contact_manager.update(group_t{g}));
        REQUIRE_EQ(contact_manager.group_count(), 2);

        sample_id = g.contact_id;
    }

    // Group was not updated - group not found
    {
        group_t g;
        g.contact_id = pfs::generate_uuid();
        g.alias = "Noname";

        REQUIRE_FALSE(contact_manager.update(std::move(g)));
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

        REQUIRE(contact_manager.add(group_t{g}));

        REQUIRE_EQ(contact_manager.group_count(), 3);

        person_t c1;
        c1.contact_id = pfs::generate_uuid();
        c1.alias = "Contact 1 for " + g.alias;

        REQUIRE(contact_manager.add(person_t{c1}));

        person_t c2;
        c2.contact_id = pfs::generate_uuid();
        c2.alias = "Contact 2 for " + g.alias;

        REQUIRE(contact_manager.add(person_t{c2}));

        // Only person can be added to group now.
        // NOTE This behavior is subject to change in the future.
        group_t c3;
        c3.contact_id = pfs::generate_uuid();
        c3.alias = "Contact 3 for " + g.alias;
        c3.creator_id = my_uuid;
        REQUIRE(contact_manager.add(group_t{c3}));

        REQUIRE(contact_manager.gref(g.contact_id).add_member(c1.contact_id));
        REQUIRE(contact_manager.gref(g.contact_id).add_member(c2.contact_id));

        REQUIRE_THROWS(contact_manager.gref(g.contact_id).add_member(c3.contact_id));

        auto members = contact_manager.gref(g.contact_id).members();
        REQUIRE(members.size() == 3);

        REQUIRE(std::find_if(members.cbegin(), members.cend(), [] (chat::contact::contact const & c) { return c.alias == my_alias; }) != members.end());
        REQUIRE(std::find_if(members.cbegin(), members.cend(), [& c1](chat::contact::contact const & c) { return c.alias == c1.alias; }) != members.end());
        REQUIRE(std::find_if(members.cbegin(), members.cend(), [& c2](chat::contact::contact const & c) { return c.alias == c2.alias; }) != members.end());

        REQUIRE(contact_manager.gref(g.contact_id).is_member_of(my_uuid));
        REQUIRE(contact_manager.gref(g.contact_id).is_member_of(c1.contact_id));
        REQUIRE(contact_manager.gref(g.contact_id).is_member_of(c2.contact_id));
        REQUIRE_FALSE(contact_manager.gref(g.contact_id).is_member_of(c3.contact_id));

        // Including my contact
        REQUIRE_EQ(contact_manager.gref(g.contact_id).count(), 3);

        auto member_ids = contact_manager.gref(g.contact_id).member_ids();
        REQUIRE_EQ(member_ids.size(), 3);
        REQUIRE(std::find(member_ids.cbegin(), member_ids.cend(), my_uuid) != member_ids.end());
        REQUIRE(std::find(member_ids.cbegin(), member_ids.cend(), c1.contact_id) != member_ids.end());
        REQUIRE(std::find(member_ids.cbegin(), member_ids.cend(), c2.contact_id) != member_ids.end());
        REQUIRE_FALSE(std::find(member_ids.cbegin(), member_ids.cend(), c3.contact_id) != member_ids.end());

        // Remove
        contact_manager.remove(c2.contact_id);
        member_ids = contact_manager.gref(g.contact_id).member_ids();
        REQUIRE_EQ(member_ids.size(), 2);

        auto removed_contact = contact_manager.get(c2.contact_id);
        REQUIRE_FALSE(is_valid(removed_contact));

        contact_manager.gref(g.contact_id).remove_member(c1.contact_id);
        REQUIRE_EQ(contact_manager.gref(g.contact_id).count(), 1);

        contact_manager.gref(g.contact_id).remove_all_members();
        REQUIRE_EQ(contact_manager.gref(g.contact_id).count(), 0);
    }

    {
        std::string my_new_alias {"My New Alias"};
        std::string my_new_avatar {"My New Avatar"};
        std::string my_new_desc {"My New Description"};
        contact_manager.change_my_alias(std::move(my_new_alias));
        contact_manager.change_my_avatar(std::move(my_new_avatar));
        contact_manager.change_my_desc(std::move(my_new_desc));

        auto me = contact_manager.my_contact();
        REQUIRE_EQ(me.alias, my_new_alias);
        REQUIRE_EQ(me.avatar, my_new_avatar);
        REQUIRE_EQ(me.description, my_new_desc);
    }
}
