################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.12.11 Refactored for using portable_target `ADD_TEST`.
################################################################################
project(chat-TESTS CXX C)

set(TESTS
    activity_manager
    contact_manager
    contact_list_search
    chat_search
    file_cache
    message_store
    messenger
    serializer)

foreach (name ${TESTS})
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE pfs::chat)
    add_test(NAME ${name} COMMAND ${name})
endforeach()

if (NOT TARGET pfs::lorem)
    set(FETCHCONTENT_UPDATES_DISCONNECTED_LOREM ON)
    include(FetchContent)

    FetchContent_Declare(lorem
        GIT_REPOSITORY "https://github.com/semenovf/lorem-lib.git"
        GIT_TAG master
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/2ndparty/lorem"
        SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/2ndparty/lorem")
    FetchContent_MakeAvailable(lorem)
endif()

target_link_libraries(contact_list_search PRIVATE pfs::lorem)

file(GENERATE OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/attachment1.bin CONTENT "ABCD")
file(GENERATE OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/attachment2.bin CONTENT "EFGH")
file(GENERATE OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/attachment3.bin CONTENT "IJKL")
