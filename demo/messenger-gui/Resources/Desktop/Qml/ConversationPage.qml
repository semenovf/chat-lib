////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.11
import chat.lib 1.0
import units.ui 1.0

Page {
    id: root

    property string inConversationWith

    header: ChatToolBar {
        ToolButton {
            text: qsTr("Back")
            anchors.left: parent.left
            anchors.leftMargin: Units.dp(10)
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.StackView.view.pop()
        }

        Label {
            id: pageTitle
            text: inConversationWith
            font.pixelSize: Units.dp(20)
            anchors.centerIn: parent
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: messageComposer.leftPadding //+ messageField.leftPadding //FIXME
            displayMarginBeginning: Units.dp(40)
            displayMarginEnd: Units.dp(40)
            verticalLayoutDirection: ListView.BottomToTop
            spacing: Units.dp(12)

            model: messengerProxy.conversationModel

            delegate: Column {
                anchors.right: sentByMe ? listView.contentItem.right : undefined
                spacing: Units.dp(6)

                readonly property bool sentByMe: model.recipient !== "Me"

                Row {
                    id: messageRow
                    spacing: Units.dp(6)
                    anchors.right: sentByMe ? parent.right : undefined

                    Image {
                        id: avatar
                        source: !sentByMe ? "qrc:/" + model.author.replace(" ", "_") + ".png" : ""
                    }

                    Rectangle {
                        width: Math.min(messageText.implicitWidth + Units.dp(24)
                            , listView.width - avatar.width - messageRow.spacing)
                        height: messageText.implicitHeight + Units.dp(24)
                        color: sentByMe ? "lightgrey" : "steelblue"

                        Label {
                            id: messageText
                            text: model.message
                            color: sentByMe ? "black" : "white"
                            anchors.fill: parent
                            anchors.margins: Units.dp(12)
                            wrapMode: Label.Wrap
                        }
                    }
                }

                Label {
                    id: timestampText
                    text: Qt.formatDateTime(model.timestamp, "d MMM hh:mm")
                    color: "lightgrey"
                    anchors.right: sentByMe ? parent.right : undefined
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }

        MessageComposer {
            id: messageComposer
            Layout.fillWidth: true
         }
    }
}

