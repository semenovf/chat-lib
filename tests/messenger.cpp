////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/chat/messenger.hpp"
#include "pfs/chat/sqlite3.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/fmt.hpp>
#include <algorithm>
#include <fstream>

namespace fs = pfs::filesystem;

namespace {

std::string TEXT {"1.Lorem ipsum dolor sit amet, consectetuer adipiscing elit,"};
std::string HTML {"<html></html>"};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// Step 1. Declare messenger
////////////////////////////////////////////////////////////////////////////////
using Messenger = chat::messenger<chat::storage::sqlite3>;

////////////////////////////////////////////////////////////////////////////////
// Step 2. Define makers for messenger components
////////////////////////////////////////////////////////////////////////////////
class MessengerEnv
{
private:
    chat::contact::person _me;
    fs::path _rootPath;

    fs::path _contactDbPath;
    fs::path _messageStoreDbPath;
    fs::path _activityManagerDbPath;
    fs::path _fileCacheDbPath;

    chat::storage::sqlite3::relational_database_t _contactDb;
    chat::storage::sqlite3::relational_database_t _messageStoreDb;
    chat::storage::sqlite3::relational_database_t _activityManagerDb;
    chat::storage::sqlite3::relational_database_t _fileCacheDb;

public:
    MessengerEnv (chat::contact::person me, fs::path rootPath)
        : _me(std::move(me))
        , _rootPath(std::move(rootPath))
    {
        _contactDbPath         = _rootPath / PFS__LITERAL_PATH("contact.db");
        _messageStoreDbPath    = _rootPath / PFS__LITERAL_PATH("messages.db");
        _activityManagerDbPath = _rootPath / PFS__LITERAL_PATH("activities.db");
        _fileCacheDbPath       = _rootPath / PFS__LITERAL_PATH("file_cache.db");
    }

public:
    fs::path rootPath () const
    {
        return _rootPath;
    }

    Messenger make ()
    {
        if (!fs::exists(_rootPath))
            fs::create_directory(_rootPath);

        _contactDb = debby::sqlite3::make(_contactDbPath);
        _messageStoreDb = debby::sqlite3::make(_messageStoreDbPath);
        _activityManagerDb = debby::sqlite3::make(_activityManagerDbPath);
        _fileCacheDb = debby::sqlite3::make(_fileCacheDbPath);

        auto contactManager  = Messenger::contact_manager_type::make(_me, _contactDb);
        auto messageStore    = Messenger::message_store_type::make(_me.contact_id, _messageStoreDb);
        auto activityManager = Messenger::activity_manager_type::make(_activityManagerDb);
        auto fileCache       = Messenger::file_cache_type::make(_fileCacheDb);

        return Messenger {
              std::move(contactManager)
            , std::move(messageStore)
            , std::move(activityManager)
            , std::move(fileCache)
        };
    }
};

TEST_CASE("messenger") {
    // Test contacts
    auto unknownContactId = pfs::generate_uuid();
    auto contactId1 = "01FV1KFY7WCBKDQZ5B4T5ZJMSA"_uuid;
    auto contactAlias1 = std::string{"PERSON_1"};
    auto contactId2 = "01FV1KFY7WWS3WSBV4BFYF7ZC9"_uuid;
    auto contactAlias2 = std::string{"PERSON_2"};
    auto contactId3 = "01G2HFKWF1MMBBXWHF4VWJGGTN"_uuid;
    auto contactAlias3 = std::string{"PERSON_3"};
    auto groupId1 = "01G2Q5AYS18JHKPTW4M8D4WYBW"_uuid;
    auto groupAlias1 = std::string{"GROUP_1"};

////////////////////////////////////////////////////////////////////////////////
// Step 3. Instantiate messenger builder
////////////////////////////////////////////////////////////////////////////////
    MessengerEnv messengerEnv1 {
          chat::contact::person {contactId1, contactAlias1}
        , fs::temp_directory_path() / fs::utf8_encode(to_string(contactId1))
    };

    MessengerEnv messengerEnv2 {
          chat::contact::person {contactId2, contactAlias2}
        , fs::temp_directory_path() / fs::utf8_encode(to_string(contactId2))
    };

////////////////////////////////////////////////////////////////////////////////
// Step 4. Instantiate messenger
////////////////////////////////////////////////////////////////////////////////
    std::vector<char> last_data_sent;

    auto send_message = [& last_data_sent] (chat::contact::id addressee
        , std::vector<char> const & data) {

        fmt::print(fmt::format("Send message {}\n", addressee));

        last_data_sent = data;
        return true;
    };

    auto messenger1 = messengerEnv1.make();
    auto messenger2 = messengerEnv2.make();

    auto received_callback = [] (chat::message::id author_id
        , chat::contact::id chat_id
        , chat::message::id message_id) {

        fmt::print("Message received from {}: {} for chat {}\n"
            , author_id, message_id, chat_id);
    };

    auto delivered_callback = [] (chat::contact::id chat_id
        , chat::message::id message_id
        , pfs::utc_time /*delivered_time*/) {

        fmt::print("Message delivered for chat {}: {}\n"
            , to_string(chat_id)
            , to_string(message_id));
    };

    auto read_callback = [] (chat::contact::id chat_id
        , chat::message::id message_id
        , pfs::utc_time /*read_time*/) {

        fmt::print("Message read for chat {}: {}\n"
            , to_string(chat_id)
            , to_string(message_id));
    };

    messenger1.dispatch_data = send_message;
    messenger2.dispatch_data = send_message;

    messenger1.message_received = received_callback;
    messenger2.message_received = received_callback;

    messenger1.message_delivered = delivered_callback;
    messenger2.message_delivered = delivered_callback;

    messenger1.message_read = read_callback;
    messenger2.message_read = read_callback;

////////////////////////////////////////////////////////////////////////////////
// Wipe before tests for clear
////////////////////////////////////////////////////////////////////////////////
    messenger1.clear_all();
    messenger2.clear_all();

////////////////////////////////////////////////////////////////////////////////
// Create files for testing attachments
////////////////////////////////////////////////////////////////////////////////
    auto f1 = messengerEnv1.rootPath() / "attachment1.bin";
    auto f2 = messengerEnv1.rootPath() / "attachment2.bin";
    std::ofstream ofs1{fs::utf8_encode(f1), std::ios::binary | std::ios::trunc};
    std::ofstream ofs2{fs::utf8_encode(f2), std::ios::binary | std::ios::trunc};

    REQUIRE(ofs1.is_open());
    REQUIRE(ofs2.is_open());

    ofs1 << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    ofs2 << "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЫЪЭЮЯ";

    ofs1.close();
    ofs2.close();

    std::size_t f1_size {26};
    std::size_t f2_size {66};

////////////////////////////////////////////////////////////////////////////////
// Step 5. Add contacts
////////////////////////////////////////////////////////////////////////////////
    REQUIRE_EQ(messenger1.cmanager().count(), 0);
    REQUIRE_EQ(messenger2.cmanager().count(), 0);

    auto contact1 = messenger1.my_contact();
    auto contact2 = messenger2.my_contact();
    chat::contact::person contact3 {contactId3, contactAlias3};

    REQUIRE_EQ(contact1.contact_id, contactId1);
    REQUIRE_EQ(contact2.contact_id, contactId2);

    REQUIRE_EQ(contact1.alias, contactAlias1);
    REQUIRE_EQ(contact2.alias, contactAlias2);

    REQUIRE_NE(messenger1.add(chat::contact::person{contact2}), chat::contact::id{});
    REQUIRE_NE(messenger2.add(chat::contact::person{contact1}), chat::contact::id{});

    REQUIRE_EQ(messenger1.add(chat::contact::person{contact2}), chat::contact::id{}); // Already exists
    REQUIRE_EQ(messenger2.add(chat::contact::person{contact1}), chat::contact::id{}); // Already exists

    REQUIRE(messenger1.update(chat::contact::person{contact2})); // Ok, attempt to update
    REQUIRE(messenger2.update(chat::contact::person{contact1})); // Ok, attempt to update

    REQUIRE_NE(messenger1.add(chat::contact::person{contact3}), chat::contact::id{});
    REQUIRE_NE(messenger2.add(chat::contact::person{contact3}), chat::contact::id{});

    REQUIRE_EQ(messenger1.cmanager().count(), 2);
    REQUIRE_EQ(messenger2.cmanager().count(), 2);

////////////////////////////////////////////////////////////////////////////////
// Step 5.1 Add group contact
////////////////////////////////////////////////////////////////////////////////
    chat::contact::group group1 {groupId1, groupAlias1, "", "", "", contactId1};
    REQUIRE_NE(messenger1.add(std::move(group1)), chat::contact::id{});
    messenger1.add_member(groupId1, contactId2);
    messenger1.add_member(groupId1, contactId3);

    REQUIRE(messenger1.is_member_of(groupId1, contactId1));
    REQUIRE(messenger1.is_member_of(groupId1, contactId2));
    REQUIRE(messenger1.is_member_of(groupId1, contactId3));
    REQUIRE_FALSE(messenger1.is_member_of(groupId1, unknownContactId));

    REQUIRE_EQ(messenger1.members_count(groupId1), 3);

    auto members = messenger1.members(groupId1);

    REQUIRE_EQ(members.size(), 3);

    REQUIRE_THROWS_AS(messenger1.members(unknownContactId), chat::error);

    auto member_ids = messenger1.member_ids(groupId1);
    REQUIRE_EQ(member_ids.size(), 3);
    CHECK_NE(std::find(member_ids.cbegin(), member_ids.cend(), contactId1), member_ids.cend());
    CHECK_NE(std::find(member_ids.cbegin(), member_ids.cend(), contactId2), member_ids.cend());
    CHECK_NE(std::find(member_ids.cbegin(), member_ids.cend(), contactId3), member_ids.cend());
    CHECK_EQ(std::find(member_ids.cbegin(), member_ids.cend(), unknownContactId), member_ids.cend());

////////////////////////////////////////////////////////////////////////////////
// Step 6.1 Write message
////////////////////////////////////////////////////////////////////////////////
    {
        // Attempt to start/continue conversation with unknown contact
        auto chat = messenger1.open_chat(unknownContactId);
        REQUIRE_FALSE(chat);

        // Assertion here since conversation is invalid
        REQUIRE_THROWS_AS(chat.create(), chat::error);
    }

    chat::message::id last_message_id;

    {
        auto chat = messenger1.open_chat(contactId2);
        REQUIRE(chat);

        auto editor = chat.create();

        REQUIRE_NE(editor.message_id(), chat::message::id{});

        editor.add_text(TEXT);
        editor.add_html(HTML);

        editor.attach(f1);
        editor.attach(f2);
        editor.save();

        last_message_id = editor.message_id();
    }

    {
        auto chat = messenger1.open_chat(contactId2);
        REQUIRE(chat);

        auto editor = chat.open(last_message_id);

        auto mime0 = editor.content().at(0).mime;

        REQUIRE_EQ(editor.content().at(0).mime, mime::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(1).mime, mime::mime_enum::text__html);
        REQUIRE_EQ(editor.content().at(2).mime, mime::mime_enum::application__octet_stream);
        REQUIRE_EQ(editor.content().at(3).mime, mime::mime_enum::application__octet_stream);

        REQUIRE_EQ(editor.content().at(0).text, TEXT);
        REQUIRE_EQ(editor.content().at(1).text, HTML);

        REQUIRE_EQ(editor.content().attachment(0).name, std::string{});
        REQUIRE_EQ(editor.content().at(2).text, fs::utf8_encode(f1.filename()));
        REQUIRE_EQ(editor.content().at(3).text, fs::utf8_encode(f2.filename()));

        REQUIRE_EQ(editor.content().attachment(2).name, fs::utf8_encode(f1.filename()));
        REQUIRE_EQ(editor.content().attachment(2).size, f1_size);

        REQUIRE_EQ(editor.content().attachment(3).name, fs::utf8_encode(f2.filename()));
        REQUIRE_EQ(editor.content().attachment(3).size, f2_size);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 6.2 Dispatch message
////////////////////////////////////////////////////////////////////////////////
    {
        auto chat = messenger1.open_chat(contactId2);
        REQUIRE(chat);

        auto m = chat.message(last_message_id);

        REQUIRE(m);
        REQUIRE_EQ(m->message_id, last_message_id);

        messenger1.dispatch_message(chat, m->message_id);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 7.1 Write group message and dispatch it
////////////////////////////////////////////////////////////////////////////////
    chat::message::id last_group_message_id;

    {
        auto chat = messenger1.open_chat(groupId1);
        REQUIRE(chat);

        auto editor = chat.create();

        editor.add_text(TEXT);
        editor.add_html(HTML);

        editor.attach(f1);
        editor.attach(f2);
        editor.save();

        last_group_message_id = editor.message_id();

        // messenger1.dispatch_message(chat, last_group_message_id);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 8.1 Receive message
////////////////////////////////////////////////////////////////////////////////
    {
        // `last_data_sent` is the serialized data sent by `messenger1`
        messenger2.process_incoming_data(contactId1, last_data_sent.data(), last_data_sent.size());
        CHECK_EQ(messenger2.unread_message_count(), 1);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 9.1 Activity manager
////////////////////////////////////////////////////////////////////////////////
    {
        auto & am1 = messenger1.amanager();
        auto & am2 = messenger2.amanager();
    }

////////////////////////////////////////////////////////////////////////////////
// Step 10.1 Message store
////////////////////////////////////////////////////////////////////////////////
    {
        auto & ms1 = messenger1.mstore();
        auto & ms2 = messenger2.mstore();
    }

////////////////////////////////////////////////////////////////////////////////
// Step 11 Other checks
////////////////////////////////////////////////////////////////////////////////
    {
        auto my_contact1 = messenger1.my_contact();
        auto my_contact2 = messenger2.my_contact();
        CHECK_EQ(my_contact1.alias, contactAlias1);
        CHECK_EQ(my_contact2.alias, contactAlias2);

        auto newAlias1 = std::string {"PERSON_1_CHANGED"};
        auto newAvatar1 = std::string {"PERSON_1_AVATAR_CHANGED"};
        auto newDesc1 = std::string{"PERSON_1_DESC_CHANGED"};
        messenger1.change_my_alias(newAlias1);
        messenger1.change_my_avatar(newAvatar1);
        messenger1.change_my_desc(newDesc1);

        my_contact1 = messenger1.my_contact();
        CHECK_EQ(my_contact1.alias, newAlias1);
        CHECK_EQ(my_contact1.avatar, newAvatar1);
        CHECK_EQ(my_contact1.description, newDesc1);
    }
}
