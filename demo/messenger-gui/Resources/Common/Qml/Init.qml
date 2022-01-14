////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
import QtQuick 2.6

Item {
    Loader {
        active: true
        source: "qrc:/QmlApplication.qml"
        asynchronous: false
        onLoaded: {
            item.visible = true;
            console.log("Init source loaded");
        }
    }
}
