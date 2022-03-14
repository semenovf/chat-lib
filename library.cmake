################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.11.17 Updated.
#      2021.11.29 Refactored for use `portable_target`.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(chat-lib C CXX)

option(CHAT__ENABLE_EXCEPTIONS "Enable exceptions for library" ON)
option(CHAT__ENABLE_JANSSON_BACKEND "Enable `Jansson` library for JSON support" ON)
option(CHAT__ENABLE_SQLITE3_CONTACT_MANAGER_BACKEND "Enable `sqlite3` contact manager backend" ON)
option(CHAT__ENABLE_SQLITE3_MESSAGE_STORE_BACKEND "Enable `sqlite3` message store backend" ON)
option(CHAT__ENABLE_NETTY_P2P_DELIVERY_MANAGER_BACKEND "Enable `netty-p2p` delivery manager backend " ON)
option(CHAT__ENABLE_CEREAL_SERIALIZER "Enable serialization based on `Cereal` library" ON)

if (CHAT__ENABLE_EXCEPTIONS)
    set(PFS__ENABLE_EXCEPTIONS ON CACHE INTERNAL "")
endif()

portable_target(LIBRARY ${PROJECT_NAME} ALIAS pfs::chat)
portable_target(SOURCES ${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/src/contact.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/emoji_db.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/mime.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/backend/delivery_manager/buffer.cpp)

if (CHAT__ENABLE_CEREAL_SERIALIZER)
    if (NOT TARGET cereal)
        portable_target(INCLUDE_PROJECT ${CMAKE_CURRENT_LIST_DIR}/cmake/Cereal.cmake)
    endif()
endif()

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/common/library.cmake)
endif()

if (CHAT__ENABLE_SQLITE3_CONTACT_MANAGER_BACKEND
        OR CHAT__ENABLE_SQLITE3_MESSAGE_STORE_BACKEND)

    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")

    if (NOT TARGET pfs::debby)
        portable_target(INCLUDE_PROJECT
            ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/debby/library.cmake)
    endif()
endif()

if (CHAT__ENABLE_NETTY_P2P_DELIVERY_MANAGER_BACKEND)
    if (NOT TARGET pfs::netty)
        portable_target(INCLUDE_PROJECT
            ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/netty/library.cmake)
    endif()

    if (NOT TARGET pfs::netty::p2p)
        portable_target(INCLUDE_PROJECT
            ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/netty/library-p2p.cmake)
    endif()

    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/delivery_manager/netty_p2p.cpp)
endif()

if (NOT TARGET pfs::jeyson)
    if (CHAT__ENABLE_JANSSON_BACKEND)
        set(JEYSON__ENABLE_JANSSON ON CACHE INTERNAL "")
        set(JEYSON__JANSSON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jansson" CACHE INTERNAL "")
    endif()

    portable_target(INCLUDE_PROJECT
        ${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/jeyson/library.cmake)
endif()

if (CHAT__ENABLE_SQLITE3_CONTACT_MANAGER_BACKEND)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/db_traits.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/contact_list.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/contact_manager.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/group_ref.cpp)
endif()

if (CHAT__ENABLE_SQLITE3_MESSAGE_STORE_BACKEND)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/db_traits.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/message_store.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/conversation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/editor.cpp)
endif()

portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::jeyson pfs::debby pfs::common)
portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::netty::p2p pfs::netty)
portable_target(EXPORTS ${PROJECT_NAME} CHAT__EXPORTS CHAT__STATIC)

if (CHAT__ENABLE_JANSSON_BACKEND)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/json_content.cpp)
    portable_target(DEFINITIONS ${PROJECT_NAME} INTERFACE "CHAT__JANSSON_BACKEND_ENABLED=1")
endif()

if (CHAT__ENABLE_CEREAL_SERIALIZER)
    portable_target(LINK ${PROJECT_NAME} PUBLIC cereal)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/cereal_serializer.cpp)
endif()
