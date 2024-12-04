################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#       2021.08.14 Initial version.
#       2021.11.17 Updated according new template.
#       2024.11.23 Up to C++14 standard.
#                  Removed `portable_target` dependency.
################################################################################
cmake_minimum_required (VERSION 3.19)
project(chat-ALL LANGUAGES CXX C)

option(CHAT__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(CHAT__BUILD_TESTS "Build tests" OFF)
option(CHAT__BUILD_DEMO "Build examples/demo" OFF)

if (CHAT__BUILD_STRICT)
    if (NOT CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 14)
    endif()

    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

if (CHAT__BUILD_TESTS OR CHAT__BUILD_DEMO)
    set(CHAT__BUILD_SHARED ON)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (CHAT__BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (CHAT__BUILD_DEMO)
    add_subdirectory(demo)
endif()

include(GNUInstallDirs)

install(TARGETS lorem
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
