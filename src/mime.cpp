////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/message.hpp"

namespace chat {
namespace message {

mime_enum to_mime (int value)
{
    switch (value) {
        case static_cast<int>(mime_enum::text__plain): return mime_enum::text__plain;
        case static_cast<int>(mime_enum::text__html): return mime_enum::text__html;
        case static_cast<int>(mime_enum::text__emoji): return mime_enum::text__emoji;
        case static_cast<int>(mime_enum::application__octet_stream): return mime_enum::application__octet_stream;
        case static_cast<int>(mime_enum::attachment): return mime_enum::attachment;
        default:
            break;
    }

    return mime_enum::invalid;
}

}} // namespace chat::message
