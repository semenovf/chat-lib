////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.2

Pane { // Since Qt 5.7 (QtQuick.Controls 2.0)
    id: pane

    RowLayout { // Since Qt 5.2 (QtQuick.Layouts 1.1)
        width: parent.width

        TextArea {
            id: messageField
            Layout.fillWidth: true
            placeholderText: qsTr("Compose message")
            wrapMode: TextArea.Wrap
        }

        Button {
            id: sendButton
            text: qsTr("Send")
            enabled: messageField.length > 0
            onClicked: {
//                 listView.model.sendMessage(inConversationWith, messageField.text);
//                 messageField.text = "";
            }
        }
    }
}
