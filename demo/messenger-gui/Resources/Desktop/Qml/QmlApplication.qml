////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.6
import QtQuick.Controls 2.2

ApplicationWindow {
    id: window
    x: 500
    width: 540
    height: 760
    visible: true

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: ContactPage {}
    }

    Component.onCompleted: {
        console.log("ApplicationWindow completed");
    }
}
