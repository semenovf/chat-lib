################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.11.17 Updated according new template.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(chat-ALL LANGUAGES CXX C)

option(CHAT__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(CHAT__BUILD_TESTS "Build tests" OFF)
option(CHAT__BUILD_DEMO "Build examples/demo" OFF)
option(CHAT__ENABLE_COVERAGE "Build tests with coverage support" OFF)
option(CHAT__ENABLE_PROFILER "Build with profiling support" OFF)

if (NOT PORTABLE_TARGET__ENABLED)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/2ndparty/portable-target/cmake/v2)
    include(PortableTarget)
    portable_target(SET ENABLE_TRACE ON)
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

if (CHAT__BUILD_STRICT)
    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

portable_target(INCLUDE_PROJECT ${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (CHAT__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

if (CHAT__BUILD_DEMO AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/demo)
    add_subdirectory(demo)
endif()

if (CHAT__ENABLE_PROFILER)
    message(STATUS "Profiler feature enabled")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "-pg")
        set(CMAKE_C_FLAGS "-pg")
    else()
        message(FATAL_ERROR "Profiler feature requires GCC")
    endif()
endif()
