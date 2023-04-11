////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/memory.hpp"
#include "pfs/chat/messenger.hpp"
#include "pfs/chat/activity_manager.hpp"
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/serializer.hpp"
#include "pfs/chat/backend/sqlite3/activity_manager.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include "pfs/chat/backend/sqlite3/message_store.hpp"
#include "pfs/chat/backend/sqlite3/file_cache.hpp"
#include "pfs/chat/backend/serializer/cereal.hpp"
#include <fstream>

namespace fs = pfs::filesystem;

namespace {

std::string TEXT {"1.Lorem ipsum dolor sit amet, consectetuer adipiscing elit,"};
std::string HTML {"<html></html>"};

auto on_failure = [] (std::string const & errstr) {
    fmt::print(stderr, "ERROR: {}\n", errstr);
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// Step 1. Declare messenger
////////////////////////////////////////////////////////////////////////////////
using Messenger = chat::messenger<
      chat::backend::sqlite3::contact_manager
    , chat::backend::sqlite3::message_store
    , chat::backend::sqlite3::activity_manager
    , chat::backend::sqlite3::file_cache
    , chat::backend::cereal::serializer>;

using SharedMessenger = std::shared_ptr<Messenger>;

////////////////////////////////////////////////////////////////////////////////
// Step 2. Define makers for messenger components
////////////////////////////////////////////////////////////////////////////////
class MessengerBuilder
{
public:
    chat::contact::person me;
    pfs::filesystem::path rootPath;

public:
    // Mandatory contact manager builder
    std::unique_ptr<Messenger::contact_manager_type> make_contact_manager () const
    {
        auto contactListPath = rootPath;

        if (!fs::exists(contactListPath)) {
            std::error_code ec;

            if (!fs::create_directory(contactListPath, ec)) {
                on_failure(fmt::format("Create directory failure: {}: {}"
                    , pfs::filesystem::utf8_encode(contactListPath)
                    , ec.message()));
                return nullptr;
            }
        }

        contactListPath /= PFS__LITERAL_PATH("contact_list.db");

        auto dbh = chat::backend::sqlite3::make_handle(contactListPath, true);

        if (!dbh)
            return nullptr;

        auto contactManager = Messenger::contact_manager_type::make_unique(me, dbh);

        if (!*contactManager)
            return nullptr;

        return contactManager;
    }

    // Mandatory message store builder
    std::unique_ptr<Messenger::message_store_type> make_message_store () const
    {
        auto messageStorePath = rootPath;

        if (!fs::exists(messageStorePath)) {
            std::error_code ec;

            if (!fs::create_directory(messageStorePath, ec)) {
                on_failure(fmt::format("Create directory failure: {}: {}"
                    , pfs::filesystem::utf8_encode(messageStorePath)
                    , ec.message()));
                return nullptr;
            }
        }

        messageStorePath /= PFS__LITERAL_PATH("messages.db");

        //auto dbh = chat::persistent_storage::sqlite3::make_handle(messageStorePath, true);
        auto dbh = chat::backend::sqlite3::make_handle(messageStorePath, true);

        if (!dbh)
            return nullptr;

        auto messageStore = Messenger::message_store_type::make_unique(me.contact_id, dbh);

        if (!*messageStore)
            return nullptr;

        return messageStore;
    }

    // Mandatory activity manager builder
    std::unique_ptr<Messenger::activity_manager_type> make_activity_manager () const
    {
        auto activityManagerPath = rootPath;

        if (!fs::exists(activityManagerPath)) {
            std::error_code ec;

            if (!fs::create_directory(activityManagerPath, ec)) {
                on_failure(fmt::format("Create directory failure: {}: {}"
                    , pfs::filesystem::utf8_encode(activityManagerPath)
                    , ec.message()));
                return nullptr;
            }
        }

        activityManagerPath /= PFS__LITERAL_PATH("activities.db");

        auto dbh = chat::backend::sqlite3::make_handle(activityManagerPath, true);

        if (!dbh)
            return nullptr;

        auto activityManager = Messenger::activity_manager_type::make_unique(dbh);

        if (!*activityManager)
            return nullptr;

        return activityManager;
    }

    // Mandatory file cache builder
    std::unique_ptr<Messenger::file_cache_type> make_file_cache () const
    {
        auto fileCachePath = rootPath;
        auto fileCacheRoot = rootPath / PFS__LITERAL_PATH("Files");

        if (!fs::exists(fileCachePath)) {
            std::error_code ec;

            if (!fs::create_directory(fileCachePath, ec)) {
                on_failure(fmt::format("Create directory failure: {}: {}"
                    , pfs::filesystem::utf8_encode(fileCachePath)
                    , ec.message()));
                return nullptr;
            }
        }

        fileCachePath /= PFS__LITERAL_PATH("file_cache.db");

        //auto dbh = chat::persistent_storage::sqlite3::make_handle(messageStorePath, true);
        auto dbh = chat::backend::sqlite3::make_handle(fileCachePath, true);

        if (!dbh)
            return nullptr;

        auto fileCache = Messenger::file_cache_type::make_unique(dbh);

        if (!*fileCache)
            return nullptr;

        return fileCache;
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
    MessengerBuilder messengerBuilder1;
    messengerBuilder1.me = chat::contact::person{contactId1, contactAlias1};
    messengerBuilder1.rootPath = fs::temp_directory_path()
        / fs::utf8_encode(to_string(contactId1));

    MessengerBuilder messengerBuilder2;
    messengerBuilder2.me = chat::contact::person{contactId2, contactAlias2};
    messengerBuilder2.rootPath = fs::temp_directory_path()
        / fs::utf8_encode(to_string(contactId2));

////////////////////////////////////////////////////////////////////////////////
// Step 4. Instantiate messenger
////////////////////////////////////////////////////////////////////////////////
    std::string last_data_sent;

    auto send_message = [& last_data_sent] (chat::contact::id addressee
        , std::string const & data) {

        fmt::print(fmt::format("Send message {}\n", addressee));

        last_data_sent = data;
        return true;
    };

    auto messenger1 = std::make_shared<Messenger>(
          messengerBuilder1.make_contact_manager()
        , messengerBuilder1.make_message_store()
        , messengerBuilder1.make_activity_manager()
        , messengerBuilder1.make_file_cache());

    auto messenger2 = std::make_shared<Messenger>(
          messengerBuilder2.make_contact_manager()
        , messengerBuilder2.make_message_store()
        , messengerBuilder2.make_activity_manager()
        , messengerBuilder2.make_file_cache());

    REQUIRE(*messenger1);
    REQUIRE(*messenger2);

    auto received_callback = [] (chat::message::id author_id
        , chat::contact::id conversation_id
        , chat::message::id message_id) {

        fmt::print("Message received from {}: {} for conversation {}\n"
            , author_id, message_id, conversation_id);
    };

    auto delivered_callback = [] (chat::contact::id conversation_id
        , chat::message::id message_id
        , pfs::utc_time_point /*delivered_time*/) {

        fmt::print("Message delivered for conversation {}: {}\n"
            , to_string(conversation_id)
            , to_string(message_id));
    };

    auto read_callback = [] (chat::contact::id conversation_id
        , chat::message::id message_id
        , pfs::utc_time_point /*read_time*/) {

        fmt::print("Message read for conversation {}: {}\n"
            , to_string(conversation_id)
            , to_string(message_id));
    };

    messenger1->dispatch_data = send_message;
    messenger2->dispatch_data = send_message;

    messenger1->message_received = received_callback;
    messenger2->message_received = received_callback;

    messenger1->message_delivered = delivered_callback;
    messenger2->message_delivered = delivered_callback;

    messenger1->message_read = read_callback;
    messenger2->message_read = read_callback;

////////////////////////////////////////////////////////////////////////////////
// Wipe before tests for clear
////////////////////////////////////////////////////////////////////////////////
    messenger1->wipe_all();
    messenger2->wipe_all();

////////////////////////////////////////////////////////////////////////////////
// Create files for testing attachments
////////////////////////////////////////////////////////////////////////////////
    auto f1 = messengerBuilder1.rootPath / "attachment1.bin";
    auto f2 = messengerBuilder1.rootPath / "attachment2.bin";
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
    REQUIRE_EQ(messenger1->contacts_count(), 0);
    REQUIRE_EQ(messenger2->contacts_count(), 0);

    auto contact1 = messenger1->my_contact();
    auto contact2 = messenger2->my_contact();
    chat::contact::person contact3 {contactId3, contactAlias3};

    REQUIRE_EQ(contact1.contact_id, contactId1);
    REQUIRE_EQ(contact2.contact_id, contactId2);

    REQUIRE_EQ(contact1.alias, contactAlias1);
    REQUIRE_EQ(contact2.alias, contactAlias2);

    REQUIRE_NE(messenger1->add(contact2), chat::contact::id{});
    REQUIRE_NE(messenger2->add(contact1), chat::contact::id{});

    REQUIRE_EQ(messenger1->add(contact2), chat::contact::id{}); // Already exists
    REQUIRE_EQ(messenger2->add(contact1), chat::contact::id{}); // Already exists

    REQUIRE(messenger1->update(contact2)); // Ok, attempt to update
    REQUIRE(messenger2->update(contact1)); // Ok, attempt to update

    REQUIRE_NE(messenger1->add(contact3), chat::contact::id{});
    REQUIRE_NE(messenger2->add(contact3), chat::contact::id{});

    REQUIRE_EQ(messenger1->contacts_count(), 2);
    REQUIRE_EQ(messenger2->contacts_count(), 2);

////////////////////////////////////////////////////////////////////////////////
// Step 5.1 Add group contact
////////////////////////////////////////////////////////////////////////////////
    chat::contact::group group1 {groupId1, groupAlias1, "", "", contactId1};
    REQUIRE_NE(messenger1->add(group1), chat::contact::id{});
    messenger1->add_member(groupId1, contactId2);
    messenger1->add_member(groupId1, contactId3);

    REQUIRE(messenger1->is_member_of(groupId1, contactId1));
    REQUIRE(messenger1->is_member_of(groupId1, contactId2));
    REQUIRE(messenger1->is_member_of(groupId1, contactId3));
    REQUIRE_FALSE(messenger1->is_member_of(groupId1, unknownContactId));

    REQUIRE_EQ(messenger1->members_count(groupId1), 3);

    auto members = messenger1->members(groupId1);

    REQUIRE_EQ(members.size(), 3);

    std::error_code ec;
    auto bad_members = messenger1->members(unknownContactId, ec);
    REQUIRE(bad_members.empty());

////////////////////////////////////////////////////////////////////////////////
// Step 6.1 Write message
////////////////////////////////////////////////////////////////////////////////
    {
        // Attempt to start/continue conversation with unknown contact
        auto conversation = messenger1->conversation(unknownContactId);
        REQUIRE_FALSE(conversation);

        // Assertion here since conversation is invalid
        // auto editor = conversation.create();
    }

    chat::message::id last_message_id;

    {
        auto conversation = messenger1->conversation(contactId2);
        REQUIRE(conversation);

        auto editor = conversation.create();

        REQUIRE_EQ(editor.message_id(), chat::message::id{});

        editor.add_text(TEXT);
        editor.add_html(HTML);

        editor.attach(f1);
        editor.attach(f2);
        editor.save();

        last_message_id = editor.message_id();
        REQUIRE_NE(editor.message_id(), chat::message::id{});
    }

    {
        auto conversation = messenger1->conversation(contactId2);
        REQUIRE(conversation);

        auto editor = conversation.open(last_message_id);

        REQUIRE_EQ(editor.content().at(0).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(1).mime, chat::message::mime_enum::text__html);
        REQUIRE_EQ(editor.content().at(2).mime, chat::message::mime_enum::application__octet_stream);
        REQUIRE_EQ(editor.content().at(3).mime, chat::message::mime_enum::application__octet_stream);

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
        auto conversation = messenger1->conversation(contactId2);
        REQUIRE(conversation);

        auto m = conversation.message(last_message_id);

        REQUIRE(m);
        REQUIRE_EQ(m->message_id, last_message_id);

        messenger1->dispatch_message(conversation, m->message_id);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 7.1 Write group message
////////////////////////////////////////////////////////////////////////////////
    chat::message::id last_group_message_id;

    {
        auto conversation = messenger1->conversation(groupId1);
        REQUIRE(conversation);

        auto editor = conversation.create();

        editor.add_text(TEXT);
        editor.add_html(HTML);

        editor.attach(f1);
        editor.attach(f2);
        editor.save();

        last_group_message_id = editor.message_id();
    }

////////////////////////////////////////////////////////////////////////////////
// Step 7.2 Dispatch group message
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Step 8.1 Receive message
////////////////////////////////////////////////////////////////////////////////
    {
        // `last_data_sent` is the serialized data sent by `messenger1`
        messenger2->process_incoming_data(contactId1, last_data_sent);
    }
}
