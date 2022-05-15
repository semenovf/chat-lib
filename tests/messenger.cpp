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
#include "pfs/chat/contact_manager.hpp"
#include "pfs/chat/message_store.hpp"
#include "pfs/chat/serializer.hpp"
#include "pfs/chat/backend/sqlite3/contact_manager.hpp"
#include "pfs/chat/backend/sqlite3/message_store.hpp"
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

        contactListPath /= "contact_list.db";

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

        messageStorePath /= "messages.db";

        //auto dbh = chat::persistent_storage::sqlite3::make_handle(messageStorePath, true);
        auto dbh = chat::backend::sqlite3::make_handle(messageStorePath, true);

        if (!dbh)
            return nullptr;

        auto messageStore = Messenger::message_store_type::make_unique(me.id, dbh);

        if (!*messageStore)
            return nullptr;

        return messageStore;
    }
};

TEST_CASE("messenger") {
    // Test contacts
    auto unknownContactId = pfs::generate_uuid();
    auto contactId1 = "01FV1KFY7WCBKDQZ5B4T5ZJMSA"_uuid;
    auto contactAlias1 = std::string{"PERSON_1"};
    auto contactId2 = "01FV1KFY7WWS3WSBV4BFYF7ZC9"_uuid;
    auto contactAlias2 = std::string{"PERSON_2"};

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

    auto send_message = [& last_data_sent] (chat::contact::contact_id addressee
        , chat::message::message_id message_id
        , std::string const & data) {

        fmt::print(fmt::format("Send message #{} to #{}\n"
            , to_string(addressee)
            , to_string(message_id)));

        last_data_sent = data;
        return true;
    };

    auto messenger1 = std::make_shared<Messenger>(
          messengerBuilder1.make_contact_manager()
        , messengerBuilder1.make_message_store());

    auto messenger2 = std::make_shared<Messenger>(
          messengerBuilder2.make_contact_manager()
        , messengerBuilder2.make_message_store());

    REQUIRE(*messenger1);
    REQUIRE(*messenger2);

    auto received_callback = [] (chat::contact::contact_id author_id
        , chat::message::message_id message_id) {

        fmt::print("Message received from #{}: #{}\n"
            , to_string(author_id)
            , to_string(message_id));
    };

    auto dispatched_callback = [] (chat::contact::contact_id addressee
        , chat::message::message_id message_id
        , pfs::utc_time_point /*dispatched_time*/) {

        fmt::print("Message dispatched for #{}: #{}\n"
            , to_string(addressee)
            , to_string(message_id));
    };

    auto delivered_callback = [] (chat::contact::contact_id addressee
        , chat::message::message_id message_id
        , pfs::utc_time_point /*delivered_time*/) {

        fmt::print("Message delivered for #{}: #{}\n"
            , to_string(addressee)
            , to_string(message_id));
    };

    auto read_callback = [] (chat::contact::contact_id addressee
        , chat::message::message_id message_id
        , pfs::utc_time_point /*read_time*/) {

        fmt::print("Message read for #{}: #{}\n"
            , to_string(addressee)
            , to_string(message_id));
    };

    messenger1->dispatch_data = send_message;
    messenger2->dispatch_data = send_message;

    messenger1->message_received = received_callback;
    messenger2->message_received = received_callback;

    messenger1->message_dispatched = dispatched_callback;
    messenger2->message_dispatched = dispatched_callback;

    messenger1->message_delivered = delivered_callback;
    messenger2->message_delivered = delivered_callback;

    messenger1->message_read = read_callback;
    messenger2->message_read = read_callback;

////////////////////////////////////////////////////////////////////////////////
// Wipe before tests for clear
////////////////////////////////////////////////////////////////////////////////
    messenger1->wipe();
    messenger2->wipe();

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

    std::string f1_sha256 {"d6ec6898de87ddac6e5b3611708a7aa1c2d298293349cc1a6c299a1db7149d38"};
    std::string f2_sha256 {"074c1bf1a31e68394b72b9a97f2ec777ec4385d9af592618bf32aabaa3b15372"};

    std::size_t f1_size {26};
    std::size_t f2_size {66};

////////////////////////////////////////////////////////////////////////////////
// Step 5. Add contacts
////////////////////////////////////////////////////////////////////////////////
    REQUIRE_EQ(messenger1->contacts_count(), 0);
    REQUIRE_EQ(messenger2->contacts_count(), 0);

    auto contact1 = messenger1->my_contact();
    auto contact2 = messenger2->my_contact();

    REQUIRE_EQ(contact1.id, contactId1);
    REQUIRE_EQ(contact2.id, contactId2);

    REQUIRE_EQ(contact1.alias, contactAlias1);
    REQUIRE_EQ(contact2.alias, contactAlias2);

    REQUIRE_NE(messenger1->add(contact2), chat::contact::contact_id{});
    REQUIRE_NE(messenger2->add(contact1), chat::contact::contact_id{});

    REQUIRE_EQ(messenger1->add(contact2), chat::contact::contact_id{}); // Already exists
    REQUIRE_EQ(messenger2->add(contact1), chat::contact::contact_id{}); // Already exists

    REQUIRE(messenger1->update(contact2)); // Ok, attempt to update
    REQUIRE(messenger2->update(contact1)); // Ok, attempt to update

    REQUIRE_EQ(messenger1->contacts_count(), 1);
    REQUIRE_EQ(messenger2->contacts_count(), 1);

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

    chat::message::message_id last_message_id;

    {
        auto conversation = messenger1->conversation(contactId2);
        REQUIRE(conversation);

        auto editor = conversation.create();
        REQUIRE(editor);

        last_message_id = editor.message_id();

        editor.add_text(TEXT);
        editor.add_html(HTML);

        editor.attach(f1);
        editor.attach(f2);
        editor.save();
    }

    {
        auto conversation = messenger1->conversation(contactId2);
        REQUIRE(conversation);

        auto editor = conversation.open(last_message_id);

        REQUIRE_EQ(editor.content().at(0).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(1).mime, chat::message::mime_enum::text__html);
        REQUIRE_EQ(editor.content().at(2).mime, chat::message::mime_enum::attachment);
        REQUIRE_EQ(editor.content().at(3).mime, chat::message::mime_enum::attachment);

        REQUIRE_EQ(editor.content().at(0).text, TEXT);
        REQUIRE_EQ(editor.content().at(1).text, HTML);

        REQUIRE_EQ(editor.content().attachment(0).name, std::string{});
        REQUIRE_EQ(editor.content().at(2).text, fs::utf8_encode(f1));
        REQUIRE_EQ(editor.content().at(3).text, fs::utf8_encode(f2));

        REQUIRE_EQ(editor.content().attachment(2).name, fs::utf8_encode(f1));
        REQUIRE_EQ(editor.content().attachment(2).size, f1_size);
        REQUIRE_EQ(editor.content().attachment(2).sha256, f1_sha256);

        REQUIRE_EQ(editor.content().attachment(3).name, fs::utf8_encode(f2));
        REQUIRE_EQ(editor.content().attachment(3).size, f2_size);
        REQUIRE_EQ(editor.content().attachment(3).sha256, f2_sha256);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 6.2 Dispatch message
////////////////////////////////////////////////////////////////////////////////
    {
        auto conversation = messenger1->conversation(contactId2);
        REQUIRE(conversation);

        auto m = conversation.message(last_message_id);

        REQUIRE(m);
        REQUIRE_EQ(m->id, last_message_id);

        messenger1->dispatch_message(contactId2, *m);
        messenger1->dispatched(contactId2, last_message_id, pfs::current_utc_time_point());
        messenger2->process_received_data(contactId1, last_data_sent);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 6.3 Receive message
////////////////////////////////////////////////////////////////////////////////
    {

    }
}
