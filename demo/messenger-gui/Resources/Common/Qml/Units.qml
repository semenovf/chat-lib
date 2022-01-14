////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.09 Initial version.
////////////////////////////////////////////////////////////////////////////////
pragma Singleton
import QtQuick 2.6

QtObject {
    function setScale (scale) { unitsBackend.scaleFactor = scale; }
    function dp (number) { return unitsBackend.dp(number) * unitsBackend.scaleFactor; }
}
