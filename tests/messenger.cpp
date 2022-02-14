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
#include "pfs/chat/persistent_storage/sqlite3/contact_manager.hpp"
#include "pfs/chat/persistent_storage/sqlite3/message_store.hpp"
#include <fstream>

namespace fs = pfs::filesystem;

namespace {

std::string TEXT1 {"1.Lorem ipsum dolor sit amet, consectetuer adipiscing elit,"};
std::string TEXT2 {"2.sed diam nonummy nibh euismod tincidunt ut laoreet dolore"};
std::string TEXT3 {"3.magna aliquam erat volutpat. Ut wisi enim ad minim veniam,"};
std::string TEXT4 {"4.quis nostrud exerci tation ullamcorper suscipit lobortis"};
std::string TEXT5 {"<html></html>"};

auto on_failure = [] (std::string const & errstr) {
    fmt::print(stderr, "ERROR: {}\n", errstr);
};

} // namespace

////////////////////////////////////////////////////////////////////////////////
// Step 1. Define messenger builder as template argument for chat::messenger
////////////////////////////////////////////////////////////////////////////////
class MessengerBuilder
{
public:
    // Mandatory type declarations
    using contact_manager_type = chat::persistent_storage::sqlite3::contact_manager;
    using message_store_type   = chat::persistent_storage::sqlite3::message_store;

public:
    chat::contact::person me;
    pfs::filesystem::path rootPath;

public:
    // Mandatory contact manager builder
    std::unique_ptr<contact_manager_type> make_contact_manager () const
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

        auto dbh = chat::persistent_storage::sqlite3::make_handle(contactListPath, true);

        if (!dbh)
            return nullptr;

        auto contactManager = pfs::make_unique<contact_manager_type>(me, dbh);

        if (!*contactManager)
            return nullptr;

        return contactManager;
    }

    // Mandatory message store builder
    std::unique_ptr<message_store_type> make_message_store () const
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

        auto dbh = chat::persistent_storage::sqlite3::make_handle(messageStorePath, true);

        if (!dbh)
            return nullptr;

        auto messageStore = pfs::make_unique<message_store_type>(dbh);

        if (!*messageStore)
            return nullptr;

        return messageStore;
    }
};

// auto message_db_path = pfs::filesystem::temp_directory_path() / "messages.db";

////////////////////////////////////////////////////////////////////////////////
// Step 2. Declare messenger
////////////////////////////////////////////////////////////////////////////////
using Messenger = chat::messenger<MessengerBuilder>;
using SharedMessenger = std::shared_ptr<Messenger>;

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
    auto messenger1 = std::make_shared<Messenger>(messengerBuilder1);
    auto messenger2 = std::make_shared<Messenger>(messengerBuilder2);

    REQUIRE(*messenger1);
    REQUIRE(*messenger2);

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

    REQUIRE(messenger1->add_contact(contact2)); // or add_or_update_contact()
    REQUIRE(messenger2->add_contact(contact1)); // or add_or_update_contact()

    REQUIRE_FALSE(messenger1->add_contact(contact2)); // Already exists
    REQUIRE_FALSE(messenger2->add_contact(contact1)); // Already exists

    REQUIRE(messenger1->add_or_update_contact(contact2)); // Ok, attempt to update
    REQUIRE(messenger2->add_or_update_contact(contact1)); // Ok, attempt to update

    REQUIRE_EQ(messenger1->contacts_count(), 1);
    REQUIRE_EQ(messenger2->contacts_count(), 1);

////////////////////////////////////////////////////////////////////////////////
// Step 6.1 Write and dispatch message
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

        editor.add_text(TEXT1);

        REQUIRE_EQ(editor.content().at(0).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(0).text, TEXT1);

        editor.add_text(TEXT2);
        editor.add_text(TEXT3);
        editor.add_text(TEXT4);
        editor.add_text(TEXT5);

        REQUIRE_EQ(editor.content().at(1).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(2).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(3).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(4).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(5).mime, chat::message::mime_enum::invalid);

        REQUIRE_EQ(editor.content().at(1).text, TEXT2);
        REQUIRE_EQ(editor.content().at(2).text, TEXT3);
        REQUIRE_EQ(editor.content().at(3).text, TEXT4);
        REQUIRE_EQ(editor.content().at(4).text, TEXT5);
        REQUIRE(editor.content().at(5).text.empty());

        REQUIRE(editor.attach(f1));
        REQUIRE(editor.attach(f2));

        chat::error err;
        REQUIRE_FALSE(editor.attach(fs::utf8_decode("ABRACADABRA"), & err));
        CHECK_EQ(err.code(), make_error_code(chat::errc::access_attachment_failure));

        REQUIRE_EQ(editor.content().at(5).mime, chat::message::mime_enum::attachment);
        REQUIRE_EQ(editor.content().at(6).mime, chat::message::mime_enum::attachment);
        REQUIRE_EQ(editor.content().at(5).text, fs::utf8_encode(f1));
        REQUIRE_EQ(editor.content().at(6).text, fs::utf8_encode(f2));

        REQUIRE(editor.save());

        // messenger1->dispatch(, [] () {});
    }

    {
        auto conversation = messenger1->conversation(contactId2);
        REQUIRE(conversation);

        auto editor = conversation.open(last_message_id);
        auto t = editor.content().at(0);

        REQUIRE_EQ(editor.content().at(0).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(1).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(2).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(3).mime, chat::message::mime_enum::text__plain);
        REQUIRE_EQ(editor.content().at(4).mime, chat::message::mime_enum::text__plain);

        REQUIRE_EQ(editor.content().at(0).text, TEXT1);
        REQUIRE_EQ(editor.content().at(1).text, TEXT2);
        REQUIRE_EQ(editor.content().at(2).text, TEXT3);
        REQUIRE_EQ(editor.content().at(3).text, TEXT4);
        REQUIRE_EQ(editor.content().at(4).text, TEXT5);
    }

////////////////////////////////////////////////////////////////////////////////
// Step 6.2 Receive message
////////////////////////////////////////////////////////////////////////////////

}
