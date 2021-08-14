################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
#
# Changelog:
#      2021.08.14 Initial version.
################################################################################
cmake_minimum_required (VERSION 3.5)
project(chat-lib CXX)

list(APPEND SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/message.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/in_memory_peer_storage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/timestamp.cpp)

# Make object files for STATIC and SHARED targets
add_library(${PROJECT_NAME}_OBJLIB OBJECT ${SOURCES})

add_library(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:${PROJECT_NAME}_OBJLIB>)
add_library(${PROJECT_NAME}-static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}_OBJLIB>)
add_library(pfs::chat ALIAS ${PROJECT_NAME})
add_library(pfs::chat::static ALIAS ${PROJECT_NAME}-static)

target_include_directories(${PROJECT_NAME}_OBJLIB PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(${PROJECT_NAME}-static PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

# if (PulseAudio_FOUND)
#     target_include_directories(${PROJECT_NAME}_OBJLIB PRIVATE ${PULSEAUDIO_INCLUDE_DIR})
#     target_include_directories(${PROJECT_NAME} PUBLIC ${PULSEAUDIO_INCLUDE_DIR})
#     target_include_directories(${PROJECT_NAME}-static PUBLIC ${PULSEAUDIO_INCLUDE_DIR})
#
#     target_link_libraries(${PROJECT_NAME}_OBJLIB PRIVATE ${PULSEAUDIO_LIBRARY})
#     target_link_libraries(${PROJECT_NAME} PUBLIC ${PULSEAUDIO_LIBRARY})
#     target_link_libraries(${PROJECT_NAME}-static PUBLIC ${PULSEAUDIO_LIBRARY})
# endif(PulseAudio_FOUND)

# Shared libraries need PIC
# For SHARED and MODULE libraries the POSITION_INDEPENDENT_CODE target property
# is set to ON automatically, but need for OBJECT type
set_property(TARGET ${PROJECT_NAME}_OBJLIB PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${PROJECT_NAME}_OBJLIB PRIVATE pfs::common)
target_link_libraries(${PROJECT_NAME} PUBLIC pfs::common)
target_link_libraries(${PROJECT_NAME}-static PUBLIC pfs::common)

if (MSVC)
    # Important! For compatiblity between STATIC and SHARED libraries
    target_compile_definitions(${PROJECT_NAME}_OBJLIB PRIVATE -DPFS_CHAT_DLL_EXPORTS)

    target_compile_definitions(${PROJECT_NAME} PUBLIC -DPFS_CHAT_DLL_EXPORTS -DUNICODE -D_UNICODE)
    target_compile_definitions(${PROJECT_NAME}-static PUBLIC -DPFS_CHAT_STATIC_LIB -DUNICODE -D_UNICODE)
endif(MSVC)
