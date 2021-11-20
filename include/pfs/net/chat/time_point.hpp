////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/fmt.hpp"
#include <chrono>
#include <string>

namespace pfs {
namespace net {
namespace chat {

using clock_type = std::chrono::system_clock;
using time_point = std::chrono::time_point<clock_type>;

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
 */
inline std::string to_iso8601_string (time_point const & t)
{
    auto millis = to_millis(t).count() % 1000;
    return fmt::format("{0:%F %H:%M:%S}.{1:03} {0:%z}", t, millis);
}

/**
 * Now it is same as to_iso8601_string()
 */
inline std::string to_string (time_point const & t)
{
    return to_iso8601_string(t);
}

}}} // namespace pfs::net::chat


