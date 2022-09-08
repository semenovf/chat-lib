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
    switch (static_cast<errc>(ev)) {
        case errc::success:
            return tr::_("no error");

        case errc::contact_not_found:
            return tr::_("contact not found");

        case errc::group_not_found:
            return tr::_("group not found");

        case errc::conversation_not_found:
            return tr::_("conversation not found");

        case errc::message_not_found:
            return tr::_("message not found");

        case errc::file_not_found:
            return tr::_("file not found");

        case errc::unsuitable_group_member:
            return tr::_("unsuitable member");

        case errc::unsuitable_group_creator:
            return tr::_("unsuitable group creator");

        case errc::group_creator_already_set:
            return tr::_("group creator already set");

        case errc::attachment_failure:
            return tr::_("attachment failure");

        case errc::bad_conversation_type:
            return tr::_("bad conversation type");

        case errc::bad_packet_type:
            return tr::_("bad packet type");

        case errc::bad_emoji_shortcode:
            return tr::_("bad Emoji shortcode");

        case errc::inconsistent_data:
            return tr::_("inconsistent data");

        case errc::filesystem_error:
            return tr::_("filesystem_error");

        case errc::storage_error:
            return tr::_("storage error");

        case errc::json_error:
            return tr::_("JSON backend error");

        default: return tr::_("unknown chat error");
    }
};

} // namespace chat
