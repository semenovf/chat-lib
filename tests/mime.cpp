////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.04.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/chat/message.hpp"

TEST_CASE("MIME by extension") {

    CHECK_EQ(chat::message::mime_by_extension("text.txt"), chat::message::mime_enum::text__plain);
    CHECK_EQ(chat::message::mime_by_extension("text.tXt"), chat::message::mime_enum::text__plain);
    CHECK_EQ(chat::message::mime_by_extension("text.LOG"), chat::message::mime_enum::text__plain);
    CHECK_EQ(chat::message::mime_by_extension("site.html"), chat::message::mime_enum::text__html);
    CHECK_EQ(chat::message::mime_by_extension("SITE.HTM"), chat::message::mime_enum::text__html);
    CHECK_EQ(chat::message::mime_by_extension("audio.ogg"), chat::message::mime_enum::audio__ogg);
    CHECK_EQ(chat::message::mime_by_extension("audio.Wav"), chat::message::mime_enum::audio__wav);
    CHECK_EQ(chat::message::mime_by_extension("video.MP4"), chat::message::mime_enum::video__mp4);
    CHECK_EQ(chat::message::mime_by_extension("image.BMP"), chat::message::mime_enum::image__bmp);
    CHECK_EQ(chat::message::mime_by_extension("image.GIF"), chat::message::mime_enum::image__gif);
    CHECK_EQ(chat::message::mime_by_extension("image.ICO"), chat::message::mime_enum::image__ico);
    CHECK_EQ(chat::message::mime_by_extension("image.jpeg"), chat::message::mime_enum::image__jpeg);
    CHECK_EQ(chat::message::mime_by_extension("image.JPG"), chat::message::mime_enum::image__jpeg);
    CHECK_EQ(chat::message::mime_by_extension("image.png"), chat::message::mime_enum::image__png);
    CHECK_EQ(chat::message::mime_by_extension("image.TIFF"), chat::message::mime_enum::image__tiff);
    CHECK_EQ(chat::message::mime_by_extension("image.tif"), chat::message::mime_enum::image__tiff);

    CHECK_EQ(chat::message::mime_by_extension(""), chat::message::mime_enum::invalid);
    CHECK_EQ(chat::message::mime_by_extension("app"), chat::message::mime_enum::application__octet_stream);
    CHECK_EQ(chat::message::mime_by_extension("app.unknown"), chat::message::mime_enum::application__octet_stream);
}
