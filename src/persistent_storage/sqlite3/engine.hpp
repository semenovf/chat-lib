////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "sqlite3.h"
#include "pfs/net/chat/time_point.hpp"
#include "pfs/filesystem.hpp"
#include <string>

namespace sqlite3_ns {

#if PFS_HAVE_STD_FILESYSTEM
namespace filesystem = std::filesystem;
#else
namespace filesystem = pfs::filesystem;
#endif

/**
 * @brief Connects to sqlite3 database.
 *
 * @params path Path to database file.
 * @return Tuple of DBI connection and an error description if error occurred.
 *         On success
 *
 * @note  Autocommit mode is on by default.
 */
sqlite3 * open (filesystem::path const & path, std::string * perrstr = nullptr);

/**
 * Close sqlite3 database connection
 */
void close (sqlite3 * dbh);

/**
 * Prepare SQL statement
 */
sqlite3_stmt * prepare (sqlite3 * dbh, std::string const & sql
    , std::string * perrstr = nullptr);

/**
 * Delete prepared statement
 */
bool finalize (sqlite3_stmt * stmt, std::string * perrstr = nullptr);

bool bind (sqlite3_stmt * stmt, std::string const & param_name
    , std::string const & value, std::string * perrstr = nullptr);
bool bind (sqlite3_stmt * stmt, std::string const & param_name
    , pfs::net::chat::time_point const & value, std::string * perrstr = nullptr);

bool step (sqlite3_stmt * stmt, std::string * perrstr = nullptr);
bool reset (sqlite3_stmt * stmt, std::string * perrstr = nullptr);

/**
 * Execute SQL query
 */
bool query (sqlite3 * dbh, std::string const & sql, std::string * perrstr = nullptr);

} // namespace sqlite3_ns
