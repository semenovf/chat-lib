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

static std::string const DEFAULT_INCOMING_TABLE_NAME { "file_cache_in" };
static std::string const DEFAULT_OUTGOING_TABLE_NAME { "file_cache_out" };

static std::string const CREATE_FILE_CACHE_TABLE {
    "CREATE TABLE IF NOT EXISTS \"{}\" ("
    "file_id {} NOT NULL"
    ", author_id {} NOT NULL"
    ", conversation_id {} NOT NULL"
    ", message_id {} NOT NULL"
    ", attachment_index {} NOT NULL"
    ", abspath {} NOT NULL"
    ", name {} NOT NULL"
    ", size {} NOT NULL"
    ", mime {} NOT NULL"
    ", modtime {} NOT NULL"
    ", PRIMARY KEY (file_id)) WITHOUT ROWID"
};

static std::string const CREATE_OUTGOING_INDEX_BY_ID {
    "CREATE UNIQUE INDEX IF NOT EXISTS \"{0}_id_index\" ON \"{0}\" (file_id)"
};

static std::string const CREATE_INCOMING_INDEX_BY_ID {
    "CREATE UNIQUE INDEX IF NOT EXISTS \"{0}_id_index\" ON \"{0}\" (file_id)"
};

namespace backend {
namespace sqlite3 {

file_cache::rep_type file_cache::make (shared_db_handle dbh)
{
    file_cache::rep_type rep;

    rep.dbh = dbh;

    rep.in_table_name  = DEFAULT_INCOMING_TABLE_NAME;
    rep.out_table_name = DEFAULT_OUTGOING_TABLE_NAME;

    std::array<std::string, 2> sqls = {
          fmt::format(CREATE_OUTGOING_INDEX_BY_ID , rep.out_table_name)
        , fmt::format(CREATE_INCOMING_INDEX_BY_ID , rep.in_table_name)
    };

    try {
        rep.dbh->begin();

        for (auto const & table_name: {rep.in_table_name, rep.out_table_name}) {
            auto sql = fmt::format(CREATE_FILE_CACHE_TABLE
                , table_name
                , affinity_traits<file::id>::name()
                , affinity_traits<contact::id>::name()
                , affinity_traits<contact::id>::name()
                , affinity_traits<message::id>::name()
                , affinity_traits<decltype(file::credentials::attachment_index)>::name()
                , affinity_traits<decltype(file::credentials::abspath)>::name()
                , affinity_traits<decltype(file::credentials::name)>::name()
                , affinity_traits<decltype(file::credentials::size)>::name()
                , affinity_traits<decltype(file::credentials::mime)>::name()
                , affinity_traits<decltype(file::credentials::modtime)>::name());

            rep.dbh->query(sql);
        }

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

static void store_file (backend::sqlite3::shared_db_handle dbh, std::string const & table_name
    , file::credentials const & fc)
{
    static std::string const INSERT_FILE {
        "INSERT OR REPLACE INTO \"{}\" (file_id, author_id, conversation_id"
            ", message_id, attachment_index, abspath, name, size, mime, modtime)"
        " VALUES (:file_id, :author_id, :conversation_id, :message_id"
            ", :attachment_index, :abspath, :name, :size, :mime, :modtime)"
    };

    int n = 0;

    try {
        auto stmt = dbh->prepare(fmt::format(INSERT_FILE, table_name));

        stmt.bind(":file_id"         , fc.file_id);
        stmt.bind(":author_id"       , fc.author_id);
        stmt.bind(":conversation_id" , fc.conversation_id);
        stmt.bind(":message_id"      , fc.message_id);
        stmt.bind(":attachment_index", fc.attachment_index);
        stmt.bind(":abspath"         , fc.abspath);
        stmt.bind(":name"            , fc.name);
        stmt.bind(":size"            , fc.size);
        stmt.bind(":mime"            , fc.mime);
        stmt.bind(":modtime"         , fc.modtime);

        auto res = stmt.exec();
        n = stmt.rows_affected();
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    if (n <= 0) {
        throw error {
              errc::storage_error
            , tr::f_("Unable to store file credentials into {}: unexpected issue"
                , table_name)
        };
    }
}

template<>
file::credentials
file_cache<BACKEND>::cache_outgoing_file (contact::id author_id
    , contact::id conversation_id, message::id message_id
    , std::int16_t attachment_index, fs::path const & path)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    file::credentials fc(author_id, conversation_id, message_id
        , attachment_index, abspath);
    store_file(_rep.dbh, _rep.out_table_name, fc);

    return fc;
}

template<>
file::credentials
file_cache<BACKEND>::cache_outgoing_file (contact::id author_id
    , contact::id conversation_id
    , message::id message_id
    , std::int16_t attachment_index
    , std::string const & uri
    , std::string const & display_name
    , std::int64_t size
    , pfs::utc_time_point modtime)
{
    file::credentials fc(author_id, conversation_id, message_id, attachment_index, uri
        , display_name, size, modtime);
    store_file(_rep.dbh, _rep.out_table_name, fc);

    return fc;
}

template<>
void
file_cache<BACKEND>::reserve_incoming_file (file::id file_id
    , contact::id author_id
    , contact::id conversation_id
    , message::id message_id
    , std::int16_t attachment_index
    , std::string const & name
    , std::size_t size
    , mime::mime_enum mime)
{
    static std::string const RESERVE_INCOMING_FILE {
        "INSERT OR REPLACE INTO \"{}\" (file_id, author_id, conversation_id"
            ", message_id, attachment_index, abspath, name, size, mime, modtime)"
        " VALUES (:file_id, :author_id, :conversation_id, :message_id"
            ", :attachment_index, :abspath, :name, :size, :mime, :modtime)"
    };

    int n = 0;

    file::credentials fc(file_id, author_id, conversation_id, message_id
        , attachment_index, name, size, mime);

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(RESERVE_INCOMING_FILE, _rep.in_table_name));

        stmt.bind(":file_id"         , fc.file_id);
        stmt.bind(":author_id"       , fc.author_id);
        stmt.bind(":conversation_id" , fc.conversation_id);
        stmt.bind(":message_id"      , fc.message_id);
        stmt.bind(":attachment_index", fc.attachment_index);
        stmt.bind(":abspath"         , fc.abspath); // invalid value (will be updated later)
        stmt.bind(":name"            , fc.name);
        stmt.bind(":size"            , fc.size);
        stmt.bind(":mime"            , fc.mime);
        stmt.bind(":modtime"         , fc.modtime); // invalid value (will be updated later)

        auto res = stmt.exec();
        n = stmt.rows_affected();
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    if (n <= 0) {
        throw error {
              errc::storage_error
            , tr::f_("Unable to reserve incoming file credentials into {}: unexpected issue"
                , _rep.in_table_name)
        };
    }
}

template<>
void
file_cache<BACKEND>::commit_incoming_file (file::id file_id
    , pfs::filesystem::path const & abspath)
{
    static std::string const COMMIT_INCOMING_FILE {
        "UPDATE \"{}\" SET abspath = :abspath, name = :name, size = :size, modtime = :modtime"
        " WHERE file_id = :file_id"
    };

    bool no_mime = true; // MIME already set by `reserve_incoming_file`.
    file::credentials fc(file_id, abspath, no_mime);

    int n = 0;

    try {
        auto stmt = _rep.dbh->prepare(fmt::format(COMMIT_INCOMING_FILE, _rep.in_table_name));

        stmt.bind(":file_id"        , fc.file_id);
        stmt.bind(":abspath"        , fc.abspath);
        stmt.bind(":name"           , fc.name);
        stmt.bind(":size"           , fc.size);
        stmt.bind(":modtime"        , fc.modtime);

        auto res = stmt.exec();
        n = stmt.rows_affected();
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    if (n <= 0) {
        throw error {
              errc::storage_error
            , tr::f_("Unable to commit incoming file credentials into {}: unexpected issue"
                , _rep.in_table_name)
        };
    }
}

static file::optional_credentials
fetch_file (file::id file_id, backend::sqlite3::shared_db_handle dbh
    , std::string const & table_name)
{
    static std::string const SELECT_FILE_BY_ID {
        "SELECT file_id, author_id, conversation_id, message_id, attachment_index"
            ", abspath, name, size, mime, modtime"
        " FROM \"{}\" WHERE file_id = :file_id"
    };

    try {
        auto stmt = dbh->prepare(fmt::format(SELECT_FILE_BY_ID, table_name));

        stmt.bind(":file_id", file_id);

        auto res = stmt.exec();

        if (res.has_more()) {
            file::credentials fc;

            res["file_id"]          >> fc.file_id;
            res["author_id"]        >> fc.author_id;
            res["conversation_id"]  >> fc.conversation_id;
            res["message_id"]       >> fc.message_id;
            res["attachment_index"] >> fc.attachment_index;
            res["abspath"]          >> fc.abspath;
            res["name"]             >> fc.name;
            res["size"]             >> fc.size;
            res["mime"]             >> fc.mime;
            res["modtime"]          >> fc.modtime;

            return fc;
        }
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    return pfs::nullopt;
}

template<>
file::optional_credentials
file_cache<BACKEND>::outgoing_file (file::id file_id) const
{
    return fetch_file(file_id, _rep.dbh, _rep.out_table_name);
}

template<>
file::optional_credentials
file_cache<BACKEND>::incoming_file (file::id file_id) const
{
    return fetch_file(file_id, _rep.dbh, _rep.in_table_name);
}

static
std::vector<file::credentials>
fetch_files (contact::id conversation_id, backend::sqlite3::shared_db_handle dbh
    , std::string const & table_name)
{
    static std::string const SELECT_FILES {
        "SELECT file_id, author_id, conversation_id, message_id, attachment_index"
            ", abspath, name, size, mime, modtime"
        " FROM \"{}\" WHERE conversation_id = :conversation_id"
    };

    std::vector<file::credentials> result;

    try {
        auto stmt = dbh->prepare(fmt::format(SELECT_FILES, table_name));

        stmt.bind(":conversation_id", conversation_id);

        auto res = stmt.exec();

        while (res.has_more()) {
            file::credentials fc;

            res["file_id"]          >> fc.file_id;
            res["author_id"]        >> fc.author_id;
            res["conversation_id"]  >> fc.conversation_id;
            res["message_id"]       >> fc.message_id;
            res["attachment_index"] >> fc.attachment_index;
            res["abspath"]          >> fc.abspath;
            res["name"]             >> fc.name;
            res["size"]             >> fc.size;
            res["mime"]             >> fc.mime;
            res["modtime"]          >> fc.modtime;

            result.push_back(std::move(fc));

            res.next();
        }
    } catch (debby::error ex) {
        throw error {errc::storage_error, ex.what()};
    }

    return result;
}

template <>
std::vector<file::credentials>
file_cache<BACKEND>::outgoing_files (contact::id conversation_id) const
{
    return fetch_files(conversation_id, _rep.dbh, _rep.out_table_name);
}

template <>
std::vector<file::credentials>
file_cache<BACKEND>::incoming_files (contact::id conversation_id) const
{
    return fetch_files(conversation_id, _rep.dbh, _rep.in_table_name);
}

static std::string const DELETE_BY_ID { "DELETE FROM \"{}\" WHERE file_id = {}" };

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

template<>
void
file_cache<BACKEND>::remove_broken ()
{
    static std::string const SELECT_PATH { "SELECT file_id, path FROM \"{}\"" };

    try {
        _rep.dbh->begin();

        for (auto const * table_name: {& _rep.out_table_name, & _rep.in_table_name}) {
            auto stmt = _rep.dbh->prepare(fmt::format(SELECT_PATH, *table_name));
            auto res = stmt.exec();

            if (res.has_more()) {
                file::id file_id;
                fs::path path;

                res["file_id"]   >> file_id;
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
    "DELETE FROM \"{}\""
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

} // namespace chat
