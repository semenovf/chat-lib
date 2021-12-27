################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.11.17 Updated.
#      2021.11.29 Refactored for use `portable_target`.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(chat-lib C CXX)

option(CHAT__ENABLE_ROCKSDB "Enable `RocksDb` library for persistent storage" OFF)

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/common/library.cmake)
endif()

if (NOT TARGET pfs::debby)
    if (CHAT__ENABLE_ROCKSDB)
        set(DEBBY__ENABLE_ROCKSDB ON CACHE INTERNAL "")
        set(DEBBY__ROCKSDB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rocksdb" CACHE INTERNAL "")
    endif()

    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/debby/library.cmake)
endif()

if (NOT TARGET pfs::net)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/net/library.cmake)
endif()

if (NOT TARGET pfs::net::p2p)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/net/library-p2p.cmake)
endif()

portable_target(LIBRARY ${PROJECT_NAME} ALIAS pfs::chat)
portable_target(SOURCES ${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/contact.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/contact_list.cpp
#     ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/file_cache.cpp
#     ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/message_store.cpp
#     ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/incoming_message_store.cpp
#     ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/outgoing_message_store.cpp
    )

portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::debby pfs::common)
portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::net::p2p pfs::net )
portable_target(EXPORTS ${PROJECT_NAME} CHAT__EXPORTS CHAT__STATIC)

if (CHAT__ENABLE_ROCKSDB)
    portable_target(DEFINITIONS ${PROJECT_NAME} INTERFACE "CHAT__ROCKSDB_ENABLED=1")
endif()
