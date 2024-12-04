################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#       2021.08.14 Initial version.
#       2021.11.17 Updated.
#       2021.11.29 Refactored for use `portable_target`.
#       2022.07.23 Configuration options replaced by `STRING` variables.
#       2023.02.10 Separated static and shared builds.
#       2024.05.18 Replaced the sequence of two target configurations with a foreach statement.
#       2024.11.23 Removed `portable_target` dependency.
################################################################################
cmake_minimum_required (VERSION 3.19)
project(chat LANGUAGES C CXX)

option(CHAT__BUILD_SHARED "Enable build shared library" OFF)
option(CHAT__ENABLE_SQLITE3_BACKEND "Enable sqlite3 storge" ON)

if (CHAT__BUILD_SHARED)
    add_library(chat SHARED)
    target_compile_definitions(chat PRIVATE CHAT__EXPORTS)
else()
    add_library(chat STATIC)
    target_compile_definitions(chat PRIVATE CHAT__STATIC)
endif()

add_library(pfs::chat ALIAS chat)

list(APPEND _chat__sources
    # FIXME
    ${CMAKE_CURRENT_LIST_DIR}/src/chat_enum.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/emoji_db.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/file.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/member_difference.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/in_memory/contact_list.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/json/content.cpp)

if (CHAT__ENABLE_SQLITE3_BACKEND)
    set(DEBBY__ENABLE_SQLITE3 ON CACHE INTERNAL "")

    list(APPEND _chat__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/activity_manager.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/contact_list.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/contact_manager.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/group_ref.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/message_store.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/chat.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/editor.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/file_cache.cpp)
endif()

set(FETCHCONTENT_UPDATES_DISCONNECTED_COMMON ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED_JEYSON ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED_DEBBY ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED_MIME ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED_IONIK ON)

include(FetchContent)

foreach (_dep common ionik jeyson mime) # debby
    if (NOT TARGET "pfs::${_dep}")
        FetchContent_Declare(${_dep}
            GIT_REPOSITORY "https://github.com/semenovf/${_dep}-lib.git"
            GIT_TAG master
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/2ndparty/${_dep}"
            SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/2ndparty/${_dep}")
        FetchContent_MakeAvailable(${_dep})
    endif()
endforeach()

# TODO Remove after implementation complete
if (NOT TARGET pfs::debby)
    FetchContent_Declare(debby
        GIT_REPOSITORY "git@github.com:semenovf/debby-lib.git"
        GIT_TAG master
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/2ndparty/debby"
        SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/2ndparty/debby")
    FetchContent_MakeAvailable(debby)
endif()

list(REMOVE_DUPLICATES _chat__sources)

target_sources(chat PRIVATE ${_chat__sources})
target_include_directories(chat PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs)
target_link_libraries(chat PUBLIC pfs::common pfs::debby pfs::jeyson pfs::mime pfs::ionik)
