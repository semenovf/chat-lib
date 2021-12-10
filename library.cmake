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

option(PFS_CHAT__ENABLE_ROCKSDB "Enable `RocksDb` library for persistent storage" OFF)

portable_target(LIBRARY ${PROJECT_NAME} ALIAS pfs::chat)
portable_target(SOURCES ${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/contact.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/contact_list.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/file_cache.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/message_store.cpp)

portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::debby pfs::common)
portable_target(LINK ${PROJECT_NAME} PRIVATE pfs::net::p2p pfs::net )
portable_target(EXPORTS ${PROJECT_NAME} PFS_CHAT__EXPORTS PFS_CHAT__STATIC)

# Set default `rocksdb` root path
if (PFS_CHAT__ENABLE_ROCKSDB AND NOT PFS_CHAT__ROCKSDB_ROOT)
    set(PFS_CHAT__ROCKSDB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rocksdb")
endif()

if (PFS_CHAT__ENABLE_ROCKSDB)
    include(FindRocksDB)

    if (PFS_ROCKSDB__LIBRARY)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PFS_ROCKSDB__LIBRARY})
        target_compile_definitions(${PROJECT_NAME} PRIVATE "-DPFS_CHAT__ROCKSDB_ENABLED=1")

        if (PFS_ROCKSDB__INCLUDE_DIR)
            target_include_directories(${PROJECT_NAME} PRIVATE ${PFS_ROCKSDB__INCLUDE_DIR})
        endif()

        message(STATUS "`RocksDB` backend enabled")
    endif()
endif()
