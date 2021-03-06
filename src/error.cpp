////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/chat/error.hpp"

namespace chat {

char const * error_category::name () const noexcept
{
    return "chat::error_category";
}

std::string error_category::message (int ev) const
{
    switch (ev) {
        case static_cast<int>(errc::success):
            return std::string{"no error"};

        case static_cast<int>(errc::contact_not_found):
            return std::string{"contact not found"};

        case static_cast<int>(errc::group_not_found):
            return std::string{"group not found"};

        case static_cast<int>(errc::message_not_found):
            return std::string{"message not found"};

        case static_cast<int>(errc::unsuitable_group_member):
            return std::string{"unsuitable member"};

        case static_cast<int>(errc::unsuitable_group_creator):
            return std::string{"unsuitable group creator"};

        case static_cast<int>(errc::group_creator_already_set):
            return std::string{"group creator already set"};

        case static_cast<int>(errc::attachment_failure):
            return std::string{"attachment failure"};

        case static_cast<int>(errc::bad_emoji_shortcode):
            return std::string{"bad Emoji shortcode"};

        case static_cast<int>(errc::inconsistent_data):
            return std::string{"inconsistent data"};

        case static_cast<int>(errc::filesystem_error):
            return std::string{"filesystem_error"};

        case static_cast<int>(errc::storage_error):
            return std::string{"storage error"};

        case static_cast<int>(errc::json_error):
            return std::string{"JSON backend error"};

        default: return std::string{"unknown chat error"};
    }
};

} // namespace debby

