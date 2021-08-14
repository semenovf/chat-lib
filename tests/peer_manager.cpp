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

    auto uuid1 = chat::generate_uuid();
    auto uuid2 = chat::generate_uuid();

    std::cout << "uuid1: " << chat::to_string(uuid1) << "\n";
    std::cout << "uuid2: " << chat::to_string(uuid2) << "\n";

    chat::peer p1;
    p1.id = uuid1;
    p1.alias = "Alias1";
    p1.icon_id = 123;

    manager.peer_connected(std::move(p1));

    chat::peer p2;
    p2.id = uuid2;
    p2.alias = "Alias2";
    p2.icon_id = 124;

    manager.peer_connected(std::move(p2));

    CHECK_EQ(manager.peers_count(), 2);

    chat::peer * p1_ptr = manager.peer_by_id(uuid1);
    chat::peer * p2_ptr = manager.peer_by_id(uuid2);

    REQUIRE_NE(p1_ptr, nullptr);
    REQUIRE_NE(p2_ptr, nullptr);

    CHECK_EQ(p1_ptr->id, uuid1);
    CHECK_EQ(p2_ptr->id, uuid2);

    manager.peer_disconnected(uuid2);

    CHECK_EQ(manager.peers_count(), 1);

    manager.peer_disconnected(uuid1);

    CHECK_EQ(manager.peers_count(), 0);
}
