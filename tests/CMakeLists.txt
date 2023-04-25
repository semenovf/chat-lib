################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.12.11 Refactored for using portable_target `ADD_TEST`.
################################################################################
project(chat-TESTS CXX C)

if (TARGET pfs::chat)
    set(_chat_lib pfs::chat)
elseif (TARGET pfs::chat::static)
    set(_chat_lib pfs::chat::static)
endif()

set(TESTS
    activity_manager
    contact_manager
    contact_list_search
    conversation_search
    file_cache
    message_store
    messenger
    mime
    serializer)

foreach (t ${TESTS})
    portable_target(ADD_TEST ${t}
        ENABLE_COVERAGE ${CHAT__ENABLE_COVERAGE}
        SOURCES ${t}.cpp ${${t}_SOURCES}
        LINK ${_chat_lib})
endforeach()

if (NOT TARGET pfs::lorem)
    portable_target(INCLUDE_PROJECT
        ${CMAKE_SOURCE_DIR}/3rdparty/pfs/lorem/library.cmake)
endif()

portable_target(LINK contact_list_search pfs::lorem)

file(GENERATE OUTPUT data/attachment1.bin CONTENT "ABCD")
file(GENERATE OUTPUT data/attachment2.bin CONTENT "EFGH")
file(GENERATE OUTPUT data/attachment3.bin CONTENT "IJKL")
