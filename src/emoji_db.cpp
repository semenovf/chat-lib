////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/emoji_db.hpp"

namespace chat {

// [Emotion Markup Language](https://en.wikipedia.org/wiki/Emotion_Markup_Language)
// [List of emoticons](https://en.wikipedia.org/wiki/List_of_emoticons)
//
// Using Emojipedia shortcodes
// https://emojipedia.org/shortcodes/
std::set<std::string> emoji_db::_emojis = {
      ":grinning_face:"                   // https://emojipedia.org/grinning-face/
    , ":grinning_face_with_big_eyes:"     // https://emojipedia.org/grinning-face-with-big-eyes/
    , ":grinning_face_with_smiling_eyes:" // https://emojipedia.org/grinning-face-with-smiling-eyes/
    , ":beaming_face_with_smiling_eyes:"  // https://emojipedia.org/beaming-face-with-smiling-eyes/
    , ":grinning_squinting_face:"         // https://emojipedia.org/grinning-squinting-face/
    , ":grinning_face_with_sweat:"        // https://emojipedia.org/grinning-face-with-sweat/
    , ":rolling_on_the_floor_laughing:"   // https://emojipedia.org/rolling-on-the-floor-laughing/
    , ":face_with_tears_of_joy:"          // https://emojipedia.org/face-with-tears-of-joy/
    , ":slightly_smiling_face:"           // https://emojipedia.org/slightly-smiling-face/
    , ":upside_down_face:"                // https://emojipedia.org/upside-down-face/
    , ":winking_face:"                    // https://emojipedia.org/winking-face/
    , ":smiling_face_with_smiling_eyes:"  // https://emojipedia.org/smiling-face-with-smiling-eyes/
    , ":smiling_face_with_halo:"          // https://emojipedia.org/smiling-face-with-halo/
    , ":smiling_face_with_hearts:"        // https://emojipedia.org/smiling-face-with-hearts/
    , ":smiling_face_with_heart_eyes:"    // https://emojipedia.org/smiling-face-with-heart-eyes/
    , ":star_struck:"                     // https://emojipedia.org/star-struck/
    , ":face_blowing_a_kiss:"             // https://emojipedia.org/face-blowing-a-kiss/
    , ":kissing_face:"                    // https://emojipedia.org/kissing-face/
    , ":smiling_face:"                    // https://emojipedia.org/smiling-face/
    , ":kissing_face_with_smiling_eyes:"  // https://emojipedia.org/kissing-face-with-smiling-eyes/
    , ":smiling_face_with_tear:"          // https://emojipedia.org/smiling-face-with-tear/
    , ":face_savoring_food:"              // https://emojipedia.org/face-savoring-food/
    , ":face_with_tongue:"                // https://emojipedia.org/face-with-tongue/
    , ":winking_face_with_tongue:"        // https://emojipedia.org/winking-face-with-tongue/
    , ":zany_face:"                       // https://emojipedia.org/zany-face/
    , ":squinting_face_with_tongue: "     // https://emojipedia.org/squinting-face-with-tongue/
    , ":thinking_face:"                   // https://emojipedia.org/thinking-face/
    , ":neutral_face:"                    // https://emojipedia.org/neutral-face/
    , ":sleeping_face:"                   // https://emojipedia.org/sleeping-face/
};

std::map<std::string, std::string> emoji_db::_emoticons = {
      { "><"  , ":grinning_squinting_face:" }
    , { "xD"  , ":grinning_squinting_face:" }
    , { "ROFL", ":rolling_on_the_floor_laughing:" }
    , { "LOL" , ":face_with_tears_of_joy:" }
    , { ":)"  , ":winking_face:" }
    , { "^^"  , ":smiling_face_with_smiling_eyes:"}
    , { ":-*" , ":kissing_face:"}
};

bool emoji_db::has (std::string const & shortcode)
{
    return _emojis.count(shortcode) > 0;
}

} // namespace chat

