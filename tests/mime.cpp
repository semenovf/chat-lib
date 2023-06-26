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

    CHECK_EQ(chat::message::mime_by_extension("text.txt"), chat::mime_enum::text__plain);
    CHECK_EQ(chat::message::mime_by_extension("text.tXt"), chat::mime_enum::text__plain);
    CHECK_EQ(chat::message::mime_by_extension("text.LOG"), chat::mime_enum::text__plain);
    CHECK_EQ(chat::message::mime_by_extension("site.html"), chat::mime_enum::text__html);
    CHECK_EQ(chat::message::mime_by_extension("SITE.HTM"), chat::mime_enum::text__html);
    CHECK_EQ(chat::message::mime_by_extension("audio.ogg"), chat::mime_enum::audio__ogg);
    CHECK_EQ(chat::message::mime_by_extension("audio.Wav"), chat::mime_enum::audio__wav);
    CHECK_EQ(chat::message::mime_by_extension("video.MP4"), chat::mime_enum::video__mp4);
    CHECK_EQ(chat::message::mime_by_extension("image.BMP"), chat::mime_enum::image__bmp);
    CHECK_EQ(chat::message::mime_by_extension("image.GIF"), chat::mime_enum::image__gif);
    CHECK_EQ(chat::message::mime_by_extension("image.ICO"), chat::mime_enum::image__ico);
    CHECK_EQ(chat::message::mime_by_extension("image.jpeg"), chat::mime_enum::image__jpeg);
    CHECK_EQ(chat::message::mime_by_extension("image.JPG"), chat::mime_enum::image__jpeg);
    CHECK_EQ(chat::message::mime_by_extension("image.png"), chat::mime_enum::image__png);
    CHECK_EQ(chat::message::mime_by_extension("image.TIFF"), chat::mime_enum::image__tiff);
    CHECK_EQ(chat::message::mime_by_extension("image.tif"), chat::mime_enum::image__tiff);

    CHECK_EQ(chat::message::mime_by_extension(""), chat::mime_enum::invalid);
    CHECK_EQ(chat::message::mime_by_extension("app"), chat::mime_enum::application__octet_stream);
    CHECK_EQ(chat::message::mime_by_extension("app.unknown"), chat::mime_enum::application__octet_stream);
}
