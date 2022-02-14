////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/error.hpp"
#include "pfs/filesystem.hpp"
#include <functional>
#include <string>
#include <system_error>
#include <cstdio>

namespace chat {

#define CHAT__ASSERT(condition, message) PFS__ASSERT(condition, message)
#define CHAT__THROW(x) PFS__THROW(x)

enum class errc
{
      success = 0
    , storage_error       // Any error of underlying storage subsystem
    , contact_not_found
    , group_not_found
    , unsuitable_member
    , access_attachment_failure
    , bad_emoji_shortcode
    , json_error         // Any error of JSON backend
};

class error_category : public std::error_category
{
public:
    virtual char const * name () const noexcept override;
    virtual std::string message (int ev) const override;
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
};

} // namespace chat
