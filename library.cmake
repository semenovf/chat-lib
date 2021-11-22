################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.11.17 Updated.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(chat-lib C CXX)

option(PFS_CHAT__ENABLE_ROCKSDB "Enable `RocksDb` library for persistent storage" OFF)
option(PFS_CHAT__ENABLE_SQLITE  "Enable `sqlite` library for persistent storage" OFF)

list(APPEND SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/time_point.cpp)

if (PFS_CHAT__ENABLE_SQLITE)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/engine.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/persistent_storage/sqlite3/contact_list.cpp)
endif()

# Make object files for STATIC and SHARED targets
add_library(${PROJECT_NAME}_OBJLIB OBJECT ${SOURCES})

add_library(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:${PROJECT_NAME}_OBJLIB>)
add_library(${PROJECT_NAME}-static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}_OBJLIB>)
add_library(pfs::chat ALIAS ${PROJECT_NAME})
add_library(pfs::chat::static ALIAS ${PROJECT_NAME}-static)

list(APPEND _link_libraries pfs::net::p2p pfs::net pfs::common Threads::Threads)

# Set default `rocksdb` root path
if (PFS_CHAT__ENABLE_ROCKSDB AND NOT PFS_CHAT__ROCKSDB_ROOT)
    set(PFS_CHAT__ROCKSDB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rocksdb")
endif()

if (PFS_CHAT__ENABLE_ROCKSDB)
    set(_ROCKSDB_ROOT ${PFS_CHAT__ROCKSDB_ROOT})
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RocksDB.cmake")

    if (_ROCKSDB_LIBRARY)
        target_link_libraries(${PROJECT_NAME}_OBJLIB PRIVATE ${_ROCKSDB_LIBRARY})

        if (_ROCKSDB_INCLUDE)
            target_include_directories(${PROJECT_NAME}_OBJLIB PRIVATE ${_ROCKSDB_INCLUDE})
        endif()

        message(STATUS "`RocksDB` backend enabled")
    else()
        message(WARNING "Unable to enable `RocksDB` backend")
    endif()
endif()

if (MSVC)
    target_compile_definitions(${PROJECT_NAME}_OBJLIB PRIVATE -DUNICODE -D_UNICODE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DUNICODE -D_UNICODE)
    target_compile_definitions(${PROJECT_NAME}-static PUBLIC -DUNICODE -D_UNICODE)
endif(MSVC)

target_include_directories(${PROJECT_NAME}_OBJLIB PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(${PROJECT_NAME}-static PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

# Shared libraries need PIC
# For SHARED and MODULE libraries the POSITION_INDEPENDENT_CODE target property
# is set to ON automatically, but need for OBJECT type
set_property(TARGET ${PROJECT_NAME}_OBJLIB PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${PROJECT_NAME}_OBJLIB PRIVATE ${_link_libraries})
target_link_libraries(${PROJECT_NAME} PUBLIC ${_link_libraries})
target_link_libraries(${PROJECT_NAME}-static PUBLIC ${_link_libraries})

if (MSVC)
    # Important! For compatiblity between STATIC and SHARED libraries
    target_compile_definitions(${PROJECT_NAME}_OBJLIB PRIVATE -DPFS_CHAT_LIB_EXPORTS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DPFS_CHAT_LIB_EXPORTS)
    target_compile_definitions(${PROJECT_NAME}-static PUBLIC -DPFS_CHAT_LIB_STATIC)
endif(MSVC)
