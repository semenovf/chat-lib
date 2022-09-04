////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/error.hpp"
#include "pfs/i18n.hpp"

namespace chat {

char const * error_category::name () const noexcept
{
    return "chat::error_category";
}

std::string error_category::message (int ev) const
{
    switch (ev) {
        case static_cast<int>(errc::success):
            return tr::_("no error");

        case static_cast<int>(errc::contact_not_found):
            return tr::_("contact not found");

        case static_cast<int>(errc::group_not_found):
            return tr::_("group not found");

        case static_cast<int>(errc::conversation_not_found):
            return tr::_("conversation not found");

        case static_cast<int>(errc::message_not_found):
            return tr::_("message not found");

        case static_cast<int>(errc::unsuitable_group_member):
            return tr::_("unsuitable member");

        case static_cast<int>(errc::unsuitable_group_creator):
            return tr::_("unsuitable group creator");

        case static_cast<int>(errc::group_creator_already_set):
            return tr::_("group creator already set");

        case static_cast<int>(errc::attachment_failure):
            return tr::_("attachment failure");

        case static_cast<int>(errc::bad_conversation_type):
            return tr::_("bad conversation type");

        case static_cast<int>(errc::bad_packet_type):
            return tr::_("bad packet type");

        case static_cast<int>(errc::bad_emoji_shortcode):
            return tr::_("bad Emoji shortcode");

        case static_cast<int>(errc::inconsistent_data):
            return tr::_("inconsistent data");

        case static_cast<int>(errc::filesystem_error):
            return tr::_("filesystem_error");

        case static_cast<int>(errc::storage_error):
            return tr::_("storage error");

        case static_cast<int>(errc::json_error):
            return tr::_("JSON backend error");

        default: return tr::_("unknown chat error");
    }
};

} // namespace chat
