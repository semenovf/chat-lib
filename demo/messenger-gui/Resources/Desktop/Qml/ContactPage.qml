////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.6
import QtQuick.Controls 2.2
import chat.lib 1.0
import units.ui 1.0

Page {
    id: root

    header: ChatToolBar {
        Label {
            text: qsTr("Contacts")
            font.pixelSize: Units.dp(20)
            anchors.centerIn: parent
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        topMargin   : 48
        leftMargin  : 48
        bottomMargin: 48
        rightMargin : 48
        spacing: 0 // Spacing between items (0 - default)
        model: messengerProxy.contactModel

        delegate: ItemDelegate {
            text: model.alias
            width: listView.width - listView.leftMargin - listView.rightMargin
            height: Units.dp(48) //avatar.implicitHeight

//             leftPadding: avatar.implicitWidth + 32
            onClicked: {
                root.StackView.view.push("qrc:/ConversationPage.qml"
                    , { inConversationWith: model.alias })
            }

//             Image {
//                 id: avatar
//                 source: "qrc:/" + model.display.replace(" ", "_") + ".png"
//             }
        }
    }
}

