################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [chat-lib}](https://github.com/semenovf/chat-lib) library.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.11.17 Updated according new template.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(pfs-chat-ALL CXX)

option(PFS_CHAT__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(PFS_CHAT__BUILD_TESTS "Build tests" OFF)
option(PFS_CHAT__BUILD_DEMO "Build examples/demo" OFF)
option(PFS_CHAT__ENABLE_COVERAGE "Build tests with coverage support" OFF)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

if (PFS_CHAT__BUILD_STRICT)
    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

if (NOT TARGET pfs::common)
    include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/common/library.cmake)
endif()

if (NOT TARGET pfs::net)
    include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/net/library.cmake)
endif()

if (NOT TARGET pfs::net::p2p)
    include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/pfs/net/library-p2p.cmake)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (PFS_CHAT__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

if (PFS_CHAT__BUILD_DEMO AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/demo)
    add_subdirectory(demo)
endif()
