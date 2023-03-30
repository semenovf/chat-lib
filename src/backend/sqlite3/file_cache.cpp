////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.06 Initial version.
//      2022.07.23 Totally refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/i18n.hpp"
#include "pfs/chat/file_cache.hpp"
#include "pfs/chat/backend/sqlite3/file_cache.hpp"
#include "pfs/debby/backend/sqlite3/path_traits.hpp"
#include "pfs/debby/backend/sqlite3/time_point_traits.hpp"

namespace chat {

using namespace debby::backend::sqlite3;
namespace fs = pfs::filesystem;

static std::string const DEFAULT_TABLE_NAME_PREFIX  { "file_cache" };

static std::string const CREATE_OUT_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`id` {} NOT NULL"
    ", `path` {} NOT NULL"
    ", `name` {} NOT NULL"
    ", `size` {} NOT NULL"
    ", `modtime` {} NOT NULL"
    ", PRIMARY KEY (`id`)) WITHOUT ROWID"
};

static std::string const CREATE_INCOMING_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`id` {} NOT NULL"
    ", `author_id` {} NOT NULL"
    ", `path` {} NOT NULL"
    ", PRIMARY KEY (`id`)) WITHOUT ROWID"
};

static std::string const CREATE_OUTGOING_INDEX_BY_ID {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{0}_id_index` ON `{0}` (`id`)"
};

static std::string const CREATE_INCOMING_INDEX_BY_ID {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{0}_id_index` ON `{0}` (`id`)"
};

namespace backend {
namespace sqlite3 {

file_cache::rep_type file_cache::make (shared_db_handle dbh)
{
    file_cache::rep_type rep;

    rep.dbh = dbh;
    rep.out_table_name = DEFAULT_TABLE_NAME_PREFIX + "_out";
    rep.in_table_name = DEFAULT_TABLE_NAME_PREFIX + "_in";

    std::array<std::string, 4> sqls = {
          fmt::format(CREATE_OUT_TABLE
            , rep.out_table_name
            , affinity_traits<file::id>::name()
            , affinity_traits<decltype(file::file_credentials::abspath)>::name()
            , affinity_traits<decltype(file::file_credentials::name)>::name()
            , affinity_traits<decltype(file::file_credentials::size)>::name()
            , affinity_traits<decltype(file::file_credentials::modtime)>::name())
        , fmt::format(CREATE_INCOMING_TABLE
            , rep.in_table_name
            , affinity_traits<file::id>::name()
            , affinity_traits<contact::id>::name()
            , affinity_traits<decltype(file::file_credentials::abspath)>::name())
        , fmt::format(CREATE_OUTGOING_INDEX_BY_ID , rep.out_table_name)
        , fmt::format(CREATE_INCOMING_INDEX_BY_ID , rep.in_table_name)
    };

    try {
        rep.dbh->begin();

        for (auto const & sql: sqls)
            rep.dbh->query(sql);

        rep.dbh->commit();
    } catch (debby::error ex) {
        rep.dbh->rollback();

        shared_db_handle empty;
        rep.dbh.swap(empty);
        throw error{errc::storage_error, ex.what()};
    }

    return rep;
}

}} // namespace backend::sqlite3

using BACKEND = backend::sqlite3::file_cache;

template <>
file_cache<BACKEND>::file_cache (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
file_cache<BACKEND>::operator bool () const noexcept
{
    return !!_rep.dbh;
}

static std::string const INSERT_OUTGOING_FILE {
    "INSERT OR REPLACE INTO `{}` (`id`, `path`, `name`, `size`, `modtime`)"
    " VALUES (:file_id, :path, :name, :size, :modtime)"
};

template<>
file::file_credentials
file_cache<BACKEND>::store_outgoing_file (std::string const & uri
    , std::string const & display_name
    , std::int64_t size
    , pfs::utc_time_point modtime)
{
    auto fc = file::make_credentials(uri, display_name, size, modtime);

    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_OUTGOING_FILE, _rep.out_table_name));

    stmt.bind(":file_id", fc.file_id);
    stmt.bind(":path"   , fc.abspath);
    stmt.bind(":name"   , fc.name);
    stmt.bind(":size"   , fc.size);
    stmt.bind(":modtime", fc.modtime);

    auto res = stmt.exec();
    auto n = stmt.rows_affected();

    if (n <= 0)
        throw error {errc::storage_error, tr::_("Expected to cache file credentials")};

    return fc;
}

template<>
file::file_credentials
file_cache<BACKEND>::store_outgoing_file (fs::path const & path)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    auto fc = file::make_credentials(abspath);

    auto stmt = _rep.dbh->prepare(fmt::format(INSERT_OUTGOING_FILE, _rep.out_table_name));

    stmt.bind(":file_id", fc.file_id);
    stmt.bind(":path"   , fc.abspath);
    stmt.bind(":name"   , fc.name);
    stmt.bind(":size"   , fc.size);
    stmt.bind(":modtime", fc.modtime);

    auto res = stmt.exec();
    auto n = stmt.rows_affected();

    if (n <= 0)
        throw error {errc::storage_error, tr::_("Expected to cache file credentials")};

    return fc;
}

static std::string const INSERT_INCOMING_FILE {
    "INSERT OR REPLACE INTO `{}` (`id`, `author_id`, `path`)"
    " VALUES (:file_id, :author_id, :path)"
};

template<>
void
file_cache<BACKEND>::store_incoming_file (contact::id author_id, file::id file_id
    , pfs::filesystem::path const & abspath)
{
    if (!abspath.is_absolute()) {
        throw error {errc::filesystem_error
            , tr::_("Path for incoming file must be an absolute")};
    }

    int n = 0;

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(INSERT_INCOMING_FILE, _rep.in_table_name));

        stmt.bind(":file_id"  , file_id);
        stmt.bind(":author_id", author_id);
        stmt.bind(":path"     , abspath);

        auto res = stmt.exec();
        n = stmt.rows_affected();
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    if (n <= 0) {
        throw error {errc::storage_error
            , tr::_("Unable to store incoming file credentials: unexpected issue")};
    }
}

std::string const SELECT_OUTGOING_FILE_BY_ID {
    "SELECT `id`, `path`, `name`, `size`, `modtime` FROM `{}` WHERE `id` = :id"
};

template<>
file::optional_file_credentials
file_cache<BACKEND>::outgoing_file (file::id file_id) const
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_OUTGOING_FILE_BY_ID
        , _rep.out_table_name));

    stmt.bind(":id", file_id);

    auto res = stmt.exec();

    if (res.has_more()) {
        file::file_credentials fc;

        res["id"]      >> fc.file_id;
        res["path"]    >> fc.abspath;
        res["name"]    >> fc.name;
        res["size"]    >> fc.size;
        res["modtime"] >> fc.modtime;

        return fc;
    }

    return pfs::nullopt;
}

std::string const SELECT_INCOMING_FILE_BY_ID {
    "SELECT `id`, `author_id`, `path` FROM `{}` WHERE `id` = :id"
};

template<>
file::optional_file_credentials
file_cache<BACKEND>::incoming_file (file::id file_id, contact::id * author_id_ptr) const
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_INCOMING_FILE_BY_ID
        , _rep.in_table_name));

    stmt.bind(":id", file_id);

    auto res = stmt.exec();

    if (res.has_more()) {
        file::file_credentials fc;

        res["id"]   >> fc.file_id;
        res["path"] >> fc.abspath;

        if (author_id_ptr)
            res["author_id"] >> *author_id_ptr;

        return fc;
    }

    return pfs::nullopt;
}

std::string const DELETE_BY_ID { "DELETE FROM `{}` WHERE `id` = {}" };

template<>
void
file_cache<BACKEND>::remove_outgoing_file (file::id file_id)
{
    auto sql = fmt::format(DELETE_BY_ID, _rep.out_table_name, file_id);
    _rep.dbh->query(sql);
}

template<>
void
file_cache<BACKEND>::remove_incoming_file (file::id file_id)
{
    auto sql = fmt::format(DELETE_BY_ID, _rep.in_table_name, file_id);
    _rep.dbh->query(sql);
}

static std::string const SELECT_PATH { "SELECT `id`, `path` FROM `{}`" };

template<>
void
file_cache<BACKEND>::remove_broken ()
{
    //std::vector<file::id> broken_ids;

    try {
        _rep.dbh->begin();

        for (auto const * table_name: {& _rep.out_table_name, & _rep.in_table_name}) {
            auto stmt = _rep.dbh->prepare(fmt::format(SELECT_PATH, *table_name));
            auto res = stmt.exec();

            if (res.has_more()) {
                file::id file_id;
                fs::path path;

                res["id"]   >> file_id;
                res["path"] >> path;

                if (!fs::exists(path)) {
                    auto sql = fmt::format(DELETE_BY_ID, *table_name, file_id);
                    _rep.dbh->query(sql);
                }
            }
        }

        _rep.dbh->commit();
    } catch (debby::error ex) {
        _rep.dbh->rollback();
        throw error{errc::storage_error, ex.what()};
    }
}

static std::string const CLEAR_TABLE {
    "DELETE FROM `{}`"
};

template<>
void
file_cache<BACKEND>::clear () noexcept
{
    for (auto const * table_name: {& _rep.out_table_name, & _rep.in_table_name}) {
        auto stmt = _rep.dbh->prepare(fmt::format(CLEAR_TABLE, *table_name));
        stmt.exec();
    }
}

static std::string const DROP_TABLE {
    "DROP TABLE IF EXISTS `{}`"
};

template<>
void
file_cache<BACKEND>::wipe () noexcept
{
    for (auto const * table_name: {& _rep.out_table_name, & _rep.in_table_name}) {
        auto stmt = _rep.dbh->prepare(fmt::format(DROP_TABLE, *table_name));
        stmt.exec();
    }
}

} // namespace chat
