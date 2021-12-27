////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.9
import QtQuick.Controls 2.2
import pfs.chat 1.0

Page {
    id: root

    header: ChatToolBar {
        Label {
            text: qsTr("Contacts")
            font.pixelSize: 20
            anchors.centerIn: parent
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        topMargin: 48
        leftMargin: 48
        bottomMargin: 48
        rightMargin: 48
        spacing: 20
        model: messengerProxyForQml.contactModel

        delegate: ItemDelegate {
            text: model.alias //model.display
            width: listView.width - listView.leftMargin - listView.rightMargin
            height: 48 //avatar.implicitHeight
//             leftPadding: avatar.implicitWidth + 32
//             onClicked: root.StackView.view.push("qrc:/ConversationPage.qml", { inConversationWith: model.display })

//             Image {
//                 id: avatar
//                 source: "qrc:/" + model.display.replace(" ", "_") + ".png"
//             }
        }
    }
}

