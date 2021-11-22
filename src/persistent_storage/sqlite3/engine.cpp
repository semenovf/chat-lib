////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "engine.hpp"
#include "pfs/fmt.hpp"
#include "pfs/windows.hpp"
#include <cassert>

namespace sqlite3_ns {

namespace {
int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second
}

sqlite3 * open (filesystem::path const & path, std::string * perrstr)
{
    int flags = 0; //SQLITE_OPEN_URI;

    // It is an error to specify a value for the mode parameter
    // that is less restrictive than that specified by the flags passed
    // in the third parameter to sqlite3_open_v2().
    flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    sqlite3 * dbh {nullptr};

    // Use default `sqlite3_vfs` object.
    char const * default_vfs = nullptr;

#if _MSC_VER
    auto utf8_path = pfs::utf8_encode(path.c_str(), std::wcslen(path.c_str()));
    int rc = sqlite3_open_v2(utf8_path.c_str(), & dbh, flags, default_vfs);
#else
    int rc = sqlite3_open_v2(path.c_str(), & dbh, flags, default_vfs);
#endif

    if (rc != SQLITE_OK) {
        std::tuple<sqlite3 *, std::string> res;

        if (!dbh) {
            // Unable to allocate memory for database handler.
            // Internal error code.
            if (perrstr)
                *perrstr = "unable to allocate memory for database handler";
        } else {
            if (perrstr)
                *perrstr = fmt::format("sqlite3_open_v2(): {}", sqlite3_errstr(rc));

            sqlite3_close_v2(dbh);
            dbh = nullptr;
        }
    } else {
        // TODO what for this call ?
        sqlite3_busy_timeout(dbh, MAX_BUSY_TIMEOUT);

        // Enable extended result codes
        sqlite3_extended_result_codes(dbh, 1);

        if (!query(dbh, "PRAGMA foreign_keys = ON", perrstr)) {
            sqlite3_close_v2(dbh);
            dbh = nullptr;
        }
    }

    return dbh;
}

void close (sqlite3 * dbh)
{
    if (dbh)
        sqlite3_close_v2(dbh);
}

sqlite3_stmt * prepare (sqlite3 * dbh, std::string const & sql, std::string * perrstr)
{
    assert(dbh);

    sqlite3_stmt * stmt {nullptr};

    auto rc = sqlite3_prepare_v2(dbh, sql.c_str(), sql.size(), & stmt, nullptr);

    if (SQLITE_OK != rc) {
        if (perrstr) {
            *perrstr = sqlite3_errmsg(dbh);
        }

        assert(stmt == nullptr);
    }

    return stmt;
}

bool finalize (sqlite3_stmt * stmt, std::string * perrstr)
{
    assert(stmt);

    auto rc = sqlite3_finalize(stmt);

    if (SQLITE_OK != rc) {
        if (perrstr)
            *perrstr = sqlite3_errstr(rc);

        return false;
    }

    return true;
}

namespace {

bool bind_helper (sqlite3_stmt * stmt, std::string const & param_name
    , std::function<int (int /*index*/)> && sqlite3_binder_wrapper
    , std::string * perrstr)
{
    assert(stmt);

    bool success = true;
    int index = sqlite3_bind_parameter_index(stmt, param_name.c_str());

    if (index == 0) {
        if (perrstr)
            *perrstr = fmt::format("bad bind parameter name: {}", param_name);
        success = false;
    }

    if (success) {
        int rc = sqlite3_binder_wrapper(index);

        if (SQLITE_OK != rc) {
            if (perrstr)
                *perrstr = sqlite3_errstr(rc);

            return false;
        }
    }

    return success;
}

} // namespace

bool bind (sqlite3_stmt * stmt, std::string const & param_name
    , std::string const & value
    , std::string * perrstr)
{
    char const * text = value.c_str();
    int len = static_cast<int>(value.size());

    return bind_helper(stmt, param_name, [stmt, text, len] (int index) {
        return sqlite3_bind_text(stmt, index, text, len, SQLITE_TRANSIENT);
    }, perrstr);
}

bool bind (sqlite3_stmt * stmt, std::string const & param_name
    , pfs::net::chat::time_point const & value, std::string * perrstr)
{
    auto tpstr = pfs::net::chat::to_iso8601(value);
    char const * text = tpstr.c_str();
    int len = static_cast<int>(tpstr.size());

    return bind_helper(stmt, param_name, [stmt, text, len] (int index) {
        return sqlite3_bind_text(stmt, index, text, len, SQLITE_TRANSIENT);
    }, perrstr);
}

bool step (sqlite3_stmt * stmt, std::string * perrstr)
{
    assert(stmt);

    int rc = sqlite3_step(stmt);
    bool success = true;

    switch (rc) {
        case SQLITE_DONE:
        case SQLITE_OK:
        case SQLITE_ROW:
            int sqlite3_reset(sqlite3_stmt *pStmt);
            break;

        default:
            if (perrstr)
                *perrstr = sqlite3_errstr(rc);
            success = false;
            break;
    }

    return success;
}

bool reset (sqlite3_stmt * stmt, std::string * perrstr)
{
    assert(stmt);

    int rc = sqlite3_reset(stmt);

    if (SQLITE_OK != rc) {
        if (perrstr)
            *perrstr = sqlite3_errstr(rc);

        return false;
    }

    return true;
}

bool query (sqlite3 * dbh, std::string const & sql, std::string * perrstr)
{
    assert(dbh);

    char * errmsg {nullptr};
    int rc = sqlite3_exec(dbh, sql.c_str(), nullptr, nullptr, & errmsg);

    if (SQLITE_OK != rc) {
        if (perrstr) {
            if (errmsg)
                *perrstr = errmsg;
            else
                *perrstr = "sqlite3_exec() failure";
        }

        return false;
    }

    return true;
}

} // namespace sqlite3_ns
