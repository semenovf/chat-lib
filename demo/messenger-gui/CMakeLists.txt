################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of `chat-lib`.
#
# Changelog:
#      2021.12.12 Initial version.
################################################################################
project(messenger-gui)

# FIXME
# find_package(Qt5 COMPONENTS Core Gui Qml Quick)
#
# if (TARGET Qt5::Core
#         AND TARGET Qt5::Gui
#         AND TARGET Qt5::Qml
#         AND TARGET Qt5::Quick)
#     portable_target(ADD_EXECUTABLE ${PROJECT_NAME})
#     portable_target(SOURCES ${PROJECT_NAME}
#         main.cpp
#         QmlApplication.cpp QmlApplication.hpp
#         ContactManagerBuilder.cpp ContactManagerBuilder.hpp
#         ControllerBuilder.cpp ContactManagerBuilder.hpp
#         ContactModel.cpp ContactModel.hpp
#         ConversationModel.cpp ConversationModel.hpp
#         MessageStoreBuilder.cpp MessageStoreBuilder.hpp
#         MessengerProxyForQml.cpp MessengerProxyForQml.hpp
#         Units.cpp Units.hpp
#         Resources/Common.qrc)
#
#     if ((MOBILE_GUI OR ANDROID) AND NOT TABLET)
#         portable_target(SOURCES ${PROJECT_NAME} Resources/Mobile.qrc)
#     else()
#         portable_target(SOURCES ${PROJECT_NAME} Resources/Desktop.qrc)
#     endif()
#
#     portable_target(LINK ${PROJECT_NAME} pfs::chat pfs::netty::p2p pfs::netty)
#     portable_target(LINK_QT5_COMPONENTS ${PROJECT_NAME}
#         Core Gui Qml Quick)
#
#     set_target_properties(${PROJECT_NAME} PROPERTIES
#         WIN32_EXECUTABLE TRUE
#         MACOSX_BUNDLE TRUE)
# else()
#     _portable_target_warn(${PROJECT_NAME} "Qt5 not found or incomplete set of \
# modules, building this demo is skipped")
# endif()
