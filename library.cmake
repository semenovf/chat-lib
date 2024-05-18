################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.11.17 Updated.
#      2021.11.29 Refactored for use `portable_target`.
#      2022.07.23 Configuration options replaced by `STRING` variables.
#      2023.02.10 Separated static and shared builds.
#      2024.05.18 Replaced the sequence of two target configurations with a foreach statement.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(chat LANGUAGES C CXX)

option(CHAT__BUILD_SHARED "Enable build shared library" OFF)
option(CHAT__BUILD_STATIC "Enable build static library" ON)

set(CHAT__CONTACT_MANAGER_BACKEND "sqlite3" CACHE STRING "Enable `sqlite3` contact manager backend")
set(CHAT__MESSAGE_STORE_BACKEND "sqlite3" CACHE STRING "Enable `sqlite3` message store backend")
set(CHAT__FILE_CACHE_BACKEND "sqlite3" CACHE STRING "Enable `sqlite3` message store backend")

if (NOT PORTABLE_TARGET__CURRENT_PROJECT_DIR)
    set(PORTABLE_TARGET__CURRENT_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (CHAT__BUILD_SHARED)
    portable_target(ADD_SHARED ${PROJECT_NAME} ALIAS pfs::chat EXPORTS CHAT__EXPORTS)
    list(APPEND _chat__targets ${PROJECT_NAME})
endif()

if (CHAT__BUILD_STATIC)
    set(STATIC_PROJECT_NAME ${PROJECT_NAME}-static)
    portable_target(ADD_STATIC ${STATIC_PROJECT_NAME} ALIAS pfs::chat::static EXPORTS CHAT__STATIC)
    list(APPEND _chat__targets ${STATIC_PROJECT_NAME})
endif()

list(APPEND _chat__sources
    ${CMAKE_CURRENT_LIST_DIR}/src/conversation_enum.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/emoji_db.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/file.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/member_difference.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/backend/in_memory/contact_list.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/backend/json/content.cpp)

if (CHAT__CONTACT_MANAGER_BACKEND STREQUAL "sqlite3")
    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")
    list(APPEND _chat__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/activity_manager.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/db_traits.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/contact_list.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/contact_manager.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/group_ref.cpp)
endif()

if (CHAT__MESSAGE_STORE_BACKEND STREQUAL "sqlite3")
    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")
    list(APPEND _chat__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/db_traits.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/message_store.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/conversation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/editor.cpp)
endif()

if (CHAT__FILE_CACHE_BACKEND STREQUAL "sqlite3")
    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")
    list(APPEND _chat__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/backend/sqlite3/file_cache.cpp)
endif()

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT ${CMAKE_CURRENT_LIST_DIR}/2ndparty/common/library.cmake)
endif()

if (NOT TARGET pfs::jeyson AND NOT TARGET pfs::jeyson::static)
    portable_target(INCLUDE_PROJECT ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/2ndparty/jeyson/library.cmake)
endif()

if (NOT TARGET pfs::debby AND NOT TARGET pfs::debby::static)
    portable_target(INCLUDE_PROJECT ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/2ndparty/debby/library.cmake)
endif()

if (NOT TARGET pfs::mime AND NOT TARGET pfs::mime::static)
    portable_target(INCLUDE_PROJECT ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/2ndparty/mime/library.cmake)
endif()

if (NOT TARGET pfs::ionik AND NOT TARGET pfs::ionik::static)
    portable_target(INCLUDE_PROJECT ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/2ndparty/ionik/library.cmake)
endif()

foreach(_target IN LISTS _chat__targets)
    portable_target(SOURCES ${_target} ${_chat__sources})
    portable_target(INCLUDE_DIRS ${_target} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/include/pfs)

    portable_target(LINK ${_target} PUBLIC pfs::common)

    if (TARGET pfs::debby)
        portable_target(LINK ${_target} PUBLIC pfs::debby)
    elseif (TARGET pfs::debby::static)
        portable_target(LINK ${_target} PUBLIC pfs::debby::static)
    endif()

    if (TARGET pfs::jeyson)
        portable_target(LINK ${_target} PUBLIC pfs::jeyson)
    elseif(TARGET pfs::jeyson::static)
        portable_target(LINK ${_target} PUBLIC pfs::jeyson::static)
    endif()

    if (TARGET pfs::mime)
        portable_target(LINK ${_target} PUBLIC pfs::mime)
    elseif(TARGET pfs::mime::static)
        portable_target(LINK ${_target} PUBLIC pfs::mime::static)
    endif()

    if (TARGET pfs::ionik)
        portable_target(LINK ${_target} PUBLIC pfs::ionik)
    elseif(TARGET pfs::ionik::static)
        portable_target(LINK ${_target} PUBLIC pfs::ionik::static)
    endif()
endforeach()
