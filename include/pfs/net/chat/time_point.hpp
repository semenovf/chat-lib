////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "optional.hpp"
#include "pfs/fmt.hpp"
#include <chrono>
#include <string>
#include <utility>

namespace pfs {
namespace net {
namespace chat {

using clock_type = std::chrono::system_clock;
using time_point = std::chrono::time_point<clock_type>;

/**
 * UTC offset in seconds
 */
std::time_t utc_offset ();

/**
 * Returns string representation of UTC offset in format [+-]HHMM
 */
std::string stringify_utc_offset (std::time_t off);

/**
 * Returns current timepoint with precision in milliseconds
 */
inline time_point current_time_point ()
{
    return std::chrono::time_point_cast<std::chrono::milliseconds>(clock_type::now());
}

inline std::chrono::milliseconds to_millis (time_point const & t)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch());
}

inline time_point from_millis (std::chrono::milliseconds const & millis)
{
    return time_point{millis};
}

/**
 * Represents time point as string according to ISO 8601 standard.
 *
 * @note May be used as Date and Time value in sqlite3.
 */
inline std::string to_iso8601 (time_point const & t)
{
    auto millis = to_millis(t).count() % 1000;
    return fmt::format("{0:%Y-%m-%dT%H:%M:%S}.{1:03}{0:%z}", t, millis);
}

/**
 * Converts ISO 8601 standard string representation
 * (in format YYYY-mm-ddTHH:MM:SS.SSS+ZZZZ) of time to time point in UTC.
 */
optional<time_point> from_iso8601 (std::string const & s);

/**
 * Now it is same as to_iso8601()
 */
inline std::string to_string (time_point const & t)
{
    return to_iso8601(t);
}

}}} // namespace pfs::net::chat


