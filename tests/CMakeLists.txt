################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.08.14 Initial version.
#      2021.12.11 Refactored for using portable_target `ADD_TEST`.
################################################################################
project(chat-lib-TESTS CXX C)

set(TESTS
    contact_manager
#     file_cache
#     message_store
    )

foreach (t ${TESTS})
    portable_target(ADD_TEST ${t}
        ENABLE_COVERAGE ${CHAT__ENABLE_COVERAGE}
        SOURCES ${t}.cpp ${${t}_SOURCES}
        LINK pfs::chat pfs::debby)
endforeach()
