////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.06 Initial version.
//      2022.07.23 Totally refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/assert.hpp"
#include "pfs/chat/file_cache.hpp"
#include "pfs/chat/backend/sqlite3/file_cache.hpp"
#include "pfs/debby/backend/sqlite3/path_traits.hpp"
#include "pfs/i18n.hpp"

namespace chat {

using namespace debby::backend::sqlite3;
namespace fs = pfs::filesystem;

static std::string const DEFAULT_TABLE_NAME  { "file_cache" };

static std::string const CREATE_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
    "`id` {} NOT NULL"
    ", `path` {} NOT NULL"
    ", `name` {} NOT NULL"
    ", `size` {} NOT NULL"
    ", `sha256` {} NOT NULL)"
};

static std::string const CREATE_INDEX_BY_ID {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{0}_id_index` ON `{0}` (`id`)"
};

static std::string const CREATE_INDEX_BY_PATH {
    "CREATE UNIQUE INDEX IF NOT EXISTS `{0}_path_index` ON `{0}` (`path`)"
};

namespace backend {
namespace sqlite3 {

file_cache::rep_type file_cache::make (pfs::filesystem::path const & root_dir
    , shared_db_handle dbh)
{
    file_cache::rep_type rep;

    rep.dbh = dbh;
    rep.table_name = DEFAULT_TABLE_NAME;
    rep.root_dir = root_dir;

    if (!fs::exists(root_dir)) {
        std::error_code ec;

        if (!fs::create_directory(root_dir, ec)) {
            throw error{errc::filesystem_error
                , tr::f_("Create directory failure: {}: {}", root_dir, ec.message())};
        }

        fs::permissions(root_dir
            , fs::perms::owner_all | fs::perms::group_all
            , fs::perm_options::replace
            , ec);

        if (ec) {
            throw error{errc::filesystem_error
                , tr::f_("Set directory permissions failure: {}: {}"
                    , root_dir, ec.message())};
        }
    }

    std::array<std::string, 3> sqls = {
          fmt::format(CREATE_TABLE
            , rep.table_name
            , affinity_traits<file::id>::name()
            , affinity_traits<decltype(file::file_credentials{}.name)>::name()
            , affinity_traits<decltype(file::file_credentials{}.name)>::name()
            , affinity_traits<decltype(file::file_credentials{}.size)>::name()
            , affinity_traits<decltype(file::file_credentials{}.sha256)>::name())
        , fmt::format(CREATE_INDEX_BY_ID , rep.table_name)
        , fmt::format(CREATE_INDEX_BY_PATH , rep.table_name)
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
std::string const SELECT_BY_ID {
    "SELECT `id`, `path`, `name`, `size`, `sha256`"
    " FROM `{}` WHERE `id` = :id"
};

template<>
file::file_credentials
file_cache<BACKEND>::load (file::id fileid)
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_BY_ID, _rep.table_name));

    PFS__ASSERT(!!stmt, "");

    stmt.bind(":id", fileid);

    auto res = stmt.exec();

    if (res.has_more()) {
        file::file_credentials fc;

        res["id"]     >> fc.fileid;
        res["path"]   >> fc.path;
        res["name"]   >> fc.name;
        res["size"]   >> fc.size;
        res["sha256"] >> fc.sha256;

        return fc;
    }

    return file::file_credentials{};
}

std::string const SELECT_BY_PATH {
    "SELECT `id`, `path`, `name`, `size`, `sha256`"
    " FROM `{}` WHERE `path` = :path"
};

template<>
file::file_credentials
file_cache<BACKEND>::load (pfs::filesystem::path const & abspath)
{
    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_BY_PATH, _rep.table_name));

    PFS__ASSERT(!!stmt, "");

    stmt.bind(":path", abspath);

    auto res = stmt.exec();

    if (res.has_more()) {
        file::file_credentials fc;

        res["id"]     >> fc.fileid;
        res["path"]   >> fc.path;
        res["name"]   >> fc.name;
        res["size"]   >> fc.size;
        res["sha256"] >> fc.sha256;

        return fc;
    }

    return file::file_credentials{};
}

std::string const DELETE_BY_ID { "DELETE FROM `{}` WHERE `id` = {}" };

template<>
void file_cache<BACKEND>::remove (file::id fileid)
{
    auto sql = fmt::format(DELETE_BY_ID, _rep.table_name, fileid);
    _rep.dbh->query(sql);
}

std::string const INSERT {
    "INSERT INTO `{}` (`id`, `path`, `name`, `size`, `sha256`)"
    " VALUES (:fileid, :path, :name, :size, :sha256)"
};

template<>
file::file_credentials
file_cache<BACKEND>::ensure (fs::path const & path, std::string const & sha256)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    auto fc = load(abspath);

    if (is_valid(fc)) {
        if (fc.sha256 == sha256)
            return fc;

        // Remove record due to checksum change
        remove(fc.fileid);
    }

    fc = file::make_credentials(abspath, sha256);

    auto stmt = _rep.dbh->prepare(fmt::format(INSERT, _rep.table_name));

    PFS__ASSERT(!!stmt, "");

    stmt.bind(":fileid", fc.fileid);
    stmt.bind(":path"  , fc.path);
    stmt.bind(":name"  , fc.name);
    stmt.bind(":size"  , fc.size);
    stmt.bind(":sha256", fc.sha256);

    auto res = stmt.exec();
    auto n = stmt.rows_affected();

    if (n > 0)
        return fc;

    return file::file_credentials{};
}

std::string const SELECT_PATH { "SELECT `id`, `path` FROM `{}`" };

template<>
std::size_t
file_cache<BACKEND>::remove_broken ()
{
    std::vector<file::id> broken_ids;

    auto stmt = _rep.dbh->prepare(fmt::format(SELECT_PATH, _rep.table_name));

    PFS__ASSERT(!!stmt, "");

    auto res = stmt.exec();

    if (res.has_more()) {
        file::id fileid;
        fs::path path;

        res["id"]   >> fileid;
        res["path"] >> path;

        if (!fs::exists(path))
            broken_ids.push_back(fileid);
    }

    if (!broken_ids.empty()) {
        try {
            _rep.dbh->begin();

            for (auto const & fileid: broken_ids) {
                auto sql = fmt::format(DELETE_BY_ID, _rep.table_name, fileid);
                _rep.dbh->query(sql);
            }

            _rep.dbh->commit();
        } catch (debby::error ex) {
            _rep.dbh->rollback();
            throw error{errc::storage_error, ex.what()};
        }
    }

    return broken_ids.size();
}

static std::string const CLEAR_TABLE {
    "DELETE FROM `{}`"
};

template<>
void
file_cache<BACKEND>::clear () noexcept
{
    auto stmt = _rep.dbh->prepare(fmt::format(CLEAR_TABLE, _rep.table_name));
    PFS__ASSERT(!!stmt, "");
    stmt.exec();
}

static std::string const DROP_TABLE {
    "DROP TABLE IF EXISTS `{}`"
};

template<>
void
file_cache<BACKEND>::wipe () noexcept
{
    auto stmt = _rep.dbh->prepare(fmt::format(DROP_TABLE, _rep.table_name));
    PFS__ASSERT(!!stmt, "");
    stmt.exec();
}

} // namespace chat
