////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include "pfs/error.hpp"
#include "pfs/filesystem.hpp"
#include <functional>
#include <string>
#include <system_error>
#include <cstdio>

namespace chat {

enum class errc
{
      success = 0
    , invalid_argument
    , contact_not_found
    , group_not_found
    , conversation_not_found
    , message_not_found
    , file_not_found
    , unsuitable_group_member
    , unsuitable_group_creator
    , group_creator_already_set
    , attachment_failure
    , bad_conversation_type
    , bad_packet_type
    , bad_emoji_shortcode
    , inconsistent_data  //
    , filesystem_error
    , storage_error      // Any error of underlying storage subsystem
    , json_error         // Any error of JSON backend
};

class error_category : public std::error_category
{
public:
    virtual CHAT__EXPORT char const * name () const noexcept override;
    virtual CHAT__EXPORT std::string message (int ev) const override;
};

inline std::error_category const & get_error_category ()
{
    static error_category instance;
    return instance;
}

inline std::error_code make_error_code (errc e)
{
    return std::error_code(static_cast<int>(e), get_error_category());
}

class error: public pfs::error
{
public:
    using pfs::error::error;

    error (errc ec)
        : pfs::error(make_error_code(ec))
    {}

    error (errc ec
        , std::string const & description
        , std::string const & cause)
        : pfs::error(make_error_code(ec), description, cause)
    {}

    error (errc ec
        , std::string const & description)
        : pfs::error(make_error_code(ec), description)
    {}

    bool ok () const
    {
        return !*this;
    }
};

using result_status = error;

} // namespace chat
