################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
#
# Changelog:
#      2021.11.17 Initial version.
#      2021.12.12 Refactored using `portable_target`.
################################################################################
project(messenger)

portable_target(ADD_EXECUTABLE ${PROJECT_NAME})
portable_target(SOURCES ${PROJECT_NAME} main.cpp)
portable_target(LINK ${PROJECT_NAME} pfs::chat pfs::net::p2p pfs::net)
