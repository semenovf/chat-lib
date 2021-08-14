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
#include "pfs/chat/in_memory_peer_storage.hpp"
#include "pfs/chat/peer_manager.hpp"

TEST_CASE("basic") {
    namespace chat = pfs::chat;
    using peer_manager = chat::peer_manager<chat::in_memory_peer_storage>;

    chat::in_memory_peer_storage storage;
    peer_manager manager {storage};

    chat::peer p1;
    p1.id = chat::uuid_t("1111");
    p1.alias = "Alias1";
    p1.icon_id = 123;

    manager.peer_connected(std::move(p1));

    chat::peer p2;
    p2.id = chat::uuid_t("2222");
    p2.alias = "Alias2";
    p2.icon_id = 124;

    manager.peer_connected(std::move(p2));

    CHECK_EQ(manager.peers_count(), 2);

    chat::peer * p1_ptr = manager.peer_by_id(chat::uuid_t("1111"));
    chat::peer * p2_ptr = manager.peer_by_id(chat::uuid_t("2222"));

    REQUIRE_NE(p1_ptr, nullptr);
    REQUIRE_NE(p2_ptr, nullptr);

    CHECK_EQ(p1_ptr->id, chat::uuid_t("1111"));
    CHECK_EQ(p2_ptr->id, chat::uuid_t("2222"));

    manager.peer_disconnected(chat::uuid_t("2222"));

    CHECK_EQ(manager.peers_count(), 1);

    manager.peer_disconnected(chat::uuid_t("1111"));

    CHECK_EQ(manager.peers_count(), 0);
}
