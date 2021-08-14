////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/chat/message.hpp"
#include "pfs/chat/peer.hpp"

TEST_CASE("basic") {
    namespace chat = pfs::chat;

    auto sender = chat::create<chat::peer>();
    sender->alias = "User1";
    sender->icon_id = 123;

    auto recipient = chat::create<chat::peer>();
    recipient->alias = "User2";
    recipient->icon_id = 124;

    auto msg = chat::create<chat::message>();

    msg->sender_id = sender->id;
    msg->receiver_id = recipient->id;

    auto text1 = chat::create<chat::text_item>();
    text1->text = "Hello";

    auto text2 = chat::create<chat::text_item>();
    text2->text = "World!";

    auto file1 = chat::create<chat::file_item>();
    file1->filename = "/tmp/file.txt";
    file1->filesize = 2048;

    chat::push(*msg, & *text1);
    chat::push(*msg, & *text2);
    chat::push(*msg, & *file1);

    std::cout << "Creation time: " << chat::to_string(msg->creation_time) << "\n";
    std::cout << "Creation time: " << chat::to_millis(msg->creation_time).count() << " millis\n";

    int text_item_counter = 2;
    int file_item_counter = 1;

    CHECK_EQ(msg->received_time.has_value(), false);
    CHECK_EQ(msg->read_time.has_value(), false);

    chat::for_each(*msg, [& text_item_counter, & file_item_counter] (
            chat::message_item const * item) {
        switch (item->type) {
            case chat::message_type::text:
                std::cout << "Text item: \""
                    << static_cast<chat::text_item const *>(item)->text
                    << "\"\n";
                text_item_counter--;
                break;

            case chat::message_type::file: {
                auto file_item = static_cast<chat::file_item const *>(item);
                std::cout << "File item: \""
                    << file_item->filename
                    << " (" << file_item->filesize << ")"
                    << "\"\n";
                file_item_counter--;
                break;
            }

            default:
                break;
        }
    });

    CHECK_EQ(text_item_counter, 0);
    CHECK_EQ(file_item_counter, 0);
}

