////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.11 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
// #include "pfs/filesystem.hpp"
// #include "pfs/fmt.hpp"
// #include "pfs/iterator.hpp"
// #include "pfs/universal_id.hpp"
#include "pfs/time_point.hpp"
#include "pfs/chat/error.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/activity_manager.hpp"
#include "pfs/chat/backend/sqlite3/activity_manager.hpp"
#include <type_traits>

// using contact_t = chat::contact::contact;
// using person_t = chat::contact::person;
// using group_t = chat::contact::group;
using activity_manager_t = chat::activity_manager<chat::backend::sqlite3::activity_manager>;

// struct forward_iterator : public pfs::iterator_facade<
//           pfs::forward_iterator_tag
//         , forward_iterator
//         , person_t, char const **, person_t>
// {
//     char const ** _p;
//     forward_iterator (char const ** p) : _p(p) {}
//
//     reference ref ()
//     {
//         person_t c;
//         c.contact_id = pfs::generate_uuid();
//         c.alias = *_p;
//         return c;
//     }
//
//     pointer ptr () { return _p; }
//     void increment (difference_type) { ++_p; }
//     bool equals (forward_iterator const & rhs) const { return _p == rhs._p;}
// };
//
// auto on_failure = [] (std::string const & errstr) {
//     fmt::print(stderr, "ERROR: {}\n", errstr);
// };

auto activity_db_path = pfs::filesystem::temp_directory_path() / "activity.db";
// auto my_uuid = pfs::generate_uuid();
// auto my_alias = std::string{"My Alias"};

REGISTER_EXCEPTION_TRANSLATOR (chat::error & ex)
{
    static std::string __s {};
    __s = ex.what();
    return doctest::String(__s.c_str(), static_cast<unsigned int>(__s.size()));
}

TEST_CASE("constructors") {
    // Public constructors/assign operators
    REQUIRE_FALSE(std::is_default_constructible<activity_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_constructible<activity_manager_t>::value);
    REQUIRE_FALSE(std::is_copy_assignable<activity_manager_t>::value);
    REQUIRE(std::is_move_constructible<activity_manager_t>::value);
    REQUIRE_FALSE(std::is_move_assignable<activity_manager_t>::value);
    REQUIRE(std::is_destructible<activity_manager_t>::value);
}

TEST_CASE("initialization") {
    if (pfs::filesystem::exists(activity_db_path)) {
        REQUIRE(pfs::filesystem::remove_all(activity_db_path) > 0);
    }

    auto dbh = chat::backend::sqlite3::make_handle(activity_db_path, true);

    REQUIRE(dbh);

    auto activity_manager = activity_manager_t::make(dbh);

    REQUIRE(activity_manager);

    activity_manager.wipe();
}

TEST_CASE("activity") {
    auto dbh = chat::backend::sqlite3::make_handle(activity_db_path, true);

    REQUIRE(dbh);

    auto activity_manager = activity_manager_t::make(dbh);

    REQUIRE(activity_manager);

    auto base_time = pfs::utc_time::from_iso8601("1972-04-29T11:00:00.000+0200");

    chat::contact::id contact_ids[] = {
          "01FV1KFY7WCBKDQZ5B4T5ZJMSA"_uuid
        , "01FV1KFY7WWS3WSBV4BFYF7ZC9"_uuid
        , "01G2HFKWF1MMBBXWHF4VWJGGTN"_uuid
    };

    chat::contact_activity activities[] = {
          chat::contact_activity::offline
        , chat::contact_activity::online
    };

    pfs::utc_time offline_utc_time = base_time;
    pfs::utc_time online_utc_time = base_time;

    int counter = 0;

    for (std::chrono::minutes time_inc = std::chrono::minutes{0}
        ; time_inc < std::chrono::minutes{60 * 12}
        ; time_inc += std::chrono::minutes{15}, counter++) {

        int index = counter % 2;
        auto time = base_time + time_inc;
        activity_manager.log_activity(contact_ids[0], activities[index], time, false);
        activity_manager.log_activity(contact_ids[1], activities[index], time, false);
        activity_manager.log_activity(contact_ids[2], activities[index], time, false);

        if (index == 0)
            offline_utc_time = time;
        else
            online_utc_time = time;
    }

    auto now = pfs::utc_time::now();
    activity_manager.log_activity(contact_ids[0], chat::contact_activity::online, now, true);

    auto last_online = activity_manager.last_activity(contact_ids[0], chat::contact_activity::online);

    REQUIRE(last_online);

    // NOTE `now` in nanoseconds precision, but time stored in milliseconds precision
    CHECK_EQ(last_online->to_millis(), now.to_millis());

    activity_manager.log_activity(contact_ids[1], chat::contact_activity::offline, now, true);
    auto last_offline = activity_manager.last_activity(contact_ids[1], chat::contact_activity::offline);
    CHECK_EQ(last_offline->to_millis(), now.to_millis());

    auto new_contact = "01FWR2WRYT8W8QT8Z9QRJ5ZTGY"_uuid;
    last_online = activity_manager.last_activity(new_contact, chat::contact_activity::online);
    last_offline = activity_manager.last_activity(new_contact, chat::contact_activity::offline);

    CHECK_FALSE(last_online);
    CHECK_FALSE(last_offline);

    auto last_activity = activity_manager.last_activity(new_contact);
    CHECK_FALSE(last_activity.online_utc_time);
    CHECK_FALSE(last_activity.offline_utc_time);

    int nrecords = 0;
    activity_manager.for_each_activity(contact_ids[0], [& nrecords] (chat::contact_activity ca
        , pfs::utc_time const & time) {
        fmt::print("{}: {}\n", to_string(time), ca == chat::contact_activity::online
            ? "ONLINE" : "OFFLINE");
        nrecords++;
    });

    CHECK_EQ(nrecords, 48);

    activity_manager.clear_activities(contact_ids[0]);
    nrecords = 0;

    activity_manager.for_each_activity(contact_ids[0], [& nrecords] (chat::contact_activity
        , pfs::utc_time const &) {
        nrecords++;
    });

    CHECK_EQ(nrecords, 0);

    nrecords = 0;

    activity_manager.for_each_activity([& nrecords] (chat::contact::id
        , chat::contact_activity
        , pfs::utc_time const &) {
        nrecords++;
    });

    CHECK_EQ(nrecords, 96);

    activity_manager.clear_activities();

    nrecords = 0;

    activity_manager.for_each_activity([& nrecords] (chat::contact::id
        , chat::contact_activity
        , pfs::utc_time const &) {
        nrecords++;
    });

    CHECK_EQ(nrecords, 0);
}
