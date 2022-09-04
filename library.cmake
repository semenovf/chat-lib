################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.11.17 Updated.
#      2021.11.29 Refactored for use `portable_target`.
#      2022.07.23 Configuration options replaced by `STRING` variables.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(chat LANGUAGES C CXX)

# Configuration variables (with default values)
set(CHAT__CONTACT_MANAGER_BACKEND "sqlite3" CACHE STRING "Enable `sqlite3` contact manager backend")
set(CHAT__MESSAGE_STORE_BACKEND "sqlite3" CACHE STRING "Enable `sqlite3` message store backend")
set(CHAT__FILE_CACHE_BACKEND "sqlite3" CACHE STRING "Enable `sqlite3` message store backend")
set(CHAT__SERIALIZER_BACKEND "cereal" CACHE STRING  "Enable serialization based on `Cereal` library")

if (NOT PORTABLE_TARGET__CURRENT_PROJECT_DIR)
    set(PORTABLE_TARGET__CURRENT_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

portable_target(ADD_SHARED ${PROJECT_NAME} ALIAS pfs::chat
    EXPORTS CHAT__EXPORTS
    BIND_STATIC ${PROJECT_NAME}-static
    STATIC_ALIAS pfs::chat::static
    STATIC_EXPORTS CHAT__STATIC)

portable_target(SOURCES ${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/src/conversation_enum.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/emoji_db.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/file.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/member_difference.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/mime.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/backend/json/content.cpp)

if (CHAT__CONTACT_MANAGER_BACKEND STREQUAL "sqlite3")
    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/db_traits.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/contact_list.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/contact_manager.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/group_ref.cpp)
endif()

if (CHAT__MESSAGE_STORE_BACKEND STREQUAL "sqlite3")
    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/db_traits.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/message_store.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/conversation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/editor.cpp)
endif()

if (CHAT__FILE_CACHE_BACKEND STREQUAL "sqlite3")
    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/file_cache.cpp)
endif()

if (CHAT__SERIALIZER_BACKEND STREQUAL "cereal")
    if (NOT TARGET cereal)
        portable_target(INCLUDE_PROJECT ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/cmake/Cereal.cmake)
    endif()

    portable_target(LINK ${PROJECT_NAME} PUBLIC cereal)
    portable_target(LINK ${PROJECT_NAME}-static PUBLIC cereal)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/serializer/cereal.cpp)
endif()

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT
        ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/3rdparty/pfs/common/library.cmake)
endif()

if (NOT TARGET pfs::jeyson)
    portable_target(INCLUDE_PROJECT
        ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/3rdparty/pfs/jeyson/library.cmake)
endif()

if (NOT TARGET pfs::debby)
    portable_target(INCLUDE_PROJECT
        ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/3rdparty/pfs/debby/library.cmake)
endif()

portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::jeyson pfs::debby pfs::common)
portable_target(LINK ${PROJECT_NAME}-static PRIVATE pfs::jeyson::static pfs::debby::static pfs::common)
