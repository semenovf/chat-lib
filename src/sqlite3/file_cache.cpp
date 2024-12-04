////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.06 Initial version.
//      2022.07.23 Totally refactored.
////////////////////////////////////////////////////////////////////////////////
#include "chat/file_cache.hpp"
#include "chat/sqlite3.hpp"
#include <pfs/i18n.hpp>
#include <pfs/debby/data_definition.hpp>
#include <pfs/debby/sqlite3.hpp>

namespace chat {

namespace fs = pfs::filesystem;

using data_definition_t = debby::data_definition<debby::backend_enum::sqlite3>;
using file_cache_t = file_cache<storage::sqlite3>;

namespace storage {

std::function<std::string ()> sqlite3::incoming_table_name = [] { return std::string{"file_cache_in"}; };
std::function<std::string ()> sqlite3::outgoing_table_name = [] { return std::string{"file_cache_out"}; };

class sqlite3::file_cache
{
public:
    relational_database_t * pdb {nullptr};
    std::string in_table_name;
    std::string out_table_name;

public:
    file_cache (relational_database_t & db)
        : in_table_name(sqlite3::incoming_table_name())
        , out_table_name(sqlite3::outgoing_table_name())
    {
        auto in = data_definition_t::create_table(in_table_name);
        auto out = data_definition_t::create_table(out_table_name);

        for (auto t: {& in, & out}) {
            t->add_column<file::id>("file_id").primary_key().unique();
            t->add_column<contact::id>("author_id");
            t->add_column<contact::id>("chat_id");
            t->add_column<message::id>("message_id");
            t->add_column<decltype(file::credentials::attachment_index)>("attachment_index");
            t->add_column<decltype(file::credentials::abspath)>("abspath");
            t->add_column<decltype(file::credentials::name)>("name");
            t->add_column<decltype(file::credentials::size)>("size");
            t->add_column<decltype(file::credentials::mime)>("mime");
            t->add_column<decltype(file::credentials::modtime)>("modtime");
            t->constraint("WITHOUT ROWID");
        }

        auto in_uindex = data_definition_t::create_index(in_table_name + "_id_uindex");
        in_uindex.unique().on(in_table_name).add_column("file_id");

        auto out_uindex = data_definition_t::create_index(out_table_name + "_id_uindex");
        out_uindex.unique().on(out_table_name).add_column("file_id");

        std::array<std::string, 4> sqls = {
            in.build(), out.build(), in_uindex.build(), out_uindex.build()
        };

        auto failure = db.transaction([& sqls, & db] () {
            debby::error err;

            for (auto const & sql: sqls) {
                db.query(sql, & err);

                if (err)
                    return pfs::make_optional(std::string{err.what()});
            }

            return pfs::optional<std::string>{};
        });

        if (failure) {
            throw error {
                  errc::storage_error
                , tr::_("create file cache failure")
                , failure.value()
            };
        }

        pdb = & db;
    }

public:
    void store_file (std::string const & table_name, file::credentials const & fc)
    {
        static std::string const INSERT_FILE {
            "INSERT OR REPLACE INTO \"{}\" (file_id, author_id, chat_id"
                ", message_id, attachment_index, abspath, name, size, mime, modtime)"
            " VALUES (:file_id, :author_id, :chat_id, :message_id"
                ", :attachment_index, :abspath, :name, :size, :mime, :modtime)"
        };

        debby::error err;
        auto stmt = pdb->prepare_cached(fmt::format(INSERT_FILE, table_name), & err);

        if (!err) {
            stmt.bind(":file_id"         , fc.file_id, & err)
                && stmt.bind(":author_id"       , fc.author_id, & err)
                && stmt.bind(":chat_id"         , fc.chat_id, & err)
                && stmt.bind(":message_id"      , fc.message_id, & err)
                && stmt.bind(":attachment_index", fc.attachment_index, & err)
                && stmt.bind(":abspath"         , std::string{fc.abspath}, & err)
                && stmt.bind(":name"            , std::string{fc.name}, & err)
                && stmt.bind(":size"            , fc.size, & err)
                && stmt.bind(":mime"            , fc.mime, & err)
                && stmt.bind(":modtime"         , fc.modtime, & err);

            if (!err) {
                auto res = stmt.exec(& err);

                if (!err) {
                    auto n = res.rows_affected();

                    if (n == 0) {
                        throw error {
                              errc::storage_error
                            , tr::f_("Unable to store file credentials into {}: unexpected issue"
                            , table_name)
                        };
                    }
                }
            }
        }

        if (err)
            throw error {errc::storage_error, err.what()};
    }

    file::optional_credentials fetch_file (file::id file_id, std::string const & table_name)
    {
        static std::string const SELECT_FILE_BY_ID {
            "SELECT file_id, author_id, chat_id, message_id, attachment_index"
                ", abspath, name, size, mime, modtime"
            " FROM \"{}\" WHERE file_id = :file_id"
        };

        debby::error err;
        auto stmt = pdb->prepare_cached(fmt::format(SELECT_FILE_BY_ID, table_name), & err);

        if (!err) {
            stmt.bind(":file_id", file_id, & err);

            if (!err) {
                auto res = stmt.exec(& err);

                if (!err) {
                    if (res.has_more()) {
                        file::credentials fc;
                        fill(res, fc);
                        return fc;
                    }
                }
            }
        }

        if (err)
            throw error {errc::storage_error, err.what()};

        return pfs::nullopt;
    }

    std::vector<file::credentials> fetch_files (contact::id chat_id, std::string const & table_name)
    {
        static std::string const SELECT_FILES {
            "SELECT file_id, author_id, chat_id, message_id, attachment_index"
                ", abspath, name, size, mime, modtime"
            " FROM \"{}\" WHERE chat_id = :chat_id"
        };

        std::vector<file::credentials> result;
        debby::error err;
        auto stmt = pdb->prepare_cached(fmt::format(SELECT_FILES, table_name), & err);

        if (!err) {
            stmt.bind(":chat_id", chat_id, & err);

            if (!err) {
                auto res = stmt.exec(& err);

                if (!err) {
                    while (res.has_more()) {
                        file::credentials fc;
                        fill(res, fc);
                        result.push_back(std::move(fc));
                        res.next();
                    }
                }
            }
        }

        if (err)
            throw error {errc::storage_error, err.what()};

        return result;
    }

private: // static
    static void fill (relational_database_t::result_type & res, file::credentials & fc)
    {
        debby::error err;
        fc.file_id = res.get_or("file_id", file::id{}, & err);
        fc.author_id = res.get_or("author_id", contact::id{}, & err);
        fc.chat_id = res.get_or("chat_id", contact::id{}, & err);
        fc.message_id = res.get_or("message_id", message::id{}, & err);
        fc.attachment_index = res.get_or("attachment_index", int{-1}, & err);
        fc.abspath = res.get_or("abspath", std::string{}, & err);
        fc.name = res.get_or("name", std::string{}, & err);
        fc.size = res.get_or("size", file::filesize_t{0}, & err);
        fc.mime = res.get_or("mime", mime::mime_enum::unknown, & err);
        fc.modtime = res.get_or("modtime", pfs::utc_time{}, & err);

        if (err)
            throw error {errc::storage_error, err.what()};
    }
};

sqlite3::file_cache *
sqlite3::make_file_cache (debby::relational_database<debby::backend_enum::sqlite3> & db)
{
    return new sqlite3::file_cache(db);
}

} // namespace storage

template <>
file_cache_t::file_cache (rep * d) noexcept
    : _d(d)
{}

template <> file_cache_t::file_cache (file_cache && other) noexcept = default;
template <> file_cache_t & file_cache_t::operator = (file_cache && other) noexcept = default;
template <> file_cache_t::~file_cache () = default;


template <>
file_cache_t::operator bool () const noexcept
{
    return !!_d && _d->pdb != nullptr;
}

template <>
file::credentials file_cache_t::cache_outgoing_file (contact::id author_id
    , contact::id chat_id, message::id message_id
    , std::int16_t attachment_index, fs::path const & path)
{
    auto abspath = path.is_absolute()
        ? path
        : fs::absolute(path);

    file::credentials fc(author_id, chat_id, message_id, attachment_index, abspath);
    _d->store_file(_d->out_table_name, fc);

    return fc;
}

template<>
file::credentials file_cache_t::cache_outgoing_file (contact::id author_id
    , contact::id chat_id
    , message::id message_id
    , std::int16_t attachment_index
    , std::string const & uri
    , std::string const & display_name
    , std::int64_t size
    , pfs::utc_time_point modtime)
{
    file::credentials fc(author_id, chat_id, message_id, attachment_index, uri
        , display_name, size, modtime);
    _d->store_file(_d->out_table_name, fc);

    return fc;
}

template<>
void file_cache_t::reserve_incoming_file (file::id file_id
    , contact::id author_id
    , contact::id chat_id
    , message::id message_id
    , std::int16_t attachment_index
    , std::string const & name
    , std::size_t size
    , mime::mime_enum mime)
{
    static std::string const RESERVE_INCOMING_FILE {
        "INSERT OR REPLACE INTO \"{}\" (file_id, author_id, chat_id"
            ", message_id, attachment_index, abspath, name, size, mime, modtime)"
        " VALUES (:file_id, :author_id, :chat_id, :message_id"
            ", :attachment_index, :abspath, :name, :size, :mime, :modtime)"
    };

    int n = 0;

    file::credentials fc(file_id, author_id, chat_id, message_id
        , attachment_index, name, size, mime);

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(RESERVE_INCOMING_FILE, _d->in_table_name), & err);

    if (!err) {
        stmt.bind(":file_id"         , fc.file_id, & err)
            && stmt.bind(":author_id"       , fc.author_id, & err)
            && stmt.bind(":chat_id"         , fc.chat_id, & err)
            && stmt.bind(":message_id"      , fc.message_id, & err)
            && stmt.bind(":attachment_index", fc.attachment_index, & err)
            && stmt.bind(":abspath"         , std::move(fc.abspath), & err) // invalid value (will be updated later)
            && stmt.bind(":name"            , std::move(fc.name), & err)
            && stmt.bind(":size"            , fc.size, & err)
            && stmt.bind(":mime"            , fc.mime, & err)
            && stmt.bind(":modtime"         , fc.modtime, & err); // invalid value (will be updated later)

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                auto n = res.rows_affected();

                if (n <= 0) {
                    throw error {
                          errc::storage_error
                        , tr::f_("Unable to reserve incoming file credentials into {}: unexpected issue"
                            , _d->in_table_name)
                    };
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

template<>
void file_cache_t::commit_incoming_file (file::id file_id, pfs::filesystem::path const & abspath)
{
    static std::string const COMMIT_INCOMING_FILE {
        "UPDATE \"{}\" SET abspath = :abspath, name = :name, size = :size, modtime = :modtime"
        " WHERE file_id = :file_id"
    };

    bool no_mime = true; // MIME already set by `reserve_incoming_file`.
    file::credentials fc(file_id, abspath, no_mime);

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(COMMIT_INCOMING_FILE, _d->in_table_name), & err);

    if (!err) {
        stmt.bind(":file_id", fc.file_id, & err)
            && stmt.bind(":abspath", std::move(fc.abspath), & err)
            && stmt.bind(":name"   , std::move(fc.name), & err)
            && stmt.bind(":size"   , fc.size, & err)
            && stmt.bind(":modtime", fc.modtime, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                auto n = res.rows_affected();

                if (n == 0) {
                    throw error {
                          errc::storage_error
                        , tr::f_("Unable to commit incoming file credentials into {}: unexpected issue"
                            , _d->in_table_name)
                    };
                }
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};
}

template<>
file::optional_credentials file_cache_t::outgoing_file (file::id file_id) const
{
    return _d->fetch_file(file_id, _d->out_table_name);
}

template<>
file::optional_credentials file_cache_t::incoming_file (file::id file_id) const
{
    return _d->fetch_file(file_id, _d->in_table_name);
}


template <>
std::vector<file::credentials> file_cache_t::outgoing_files (contact::id chat_id) const
{
    return _d->fetch_files(chat_id, _d->out_table_name);
}

template <>
std::vector<file::credentials> file_cache_t::incoming_files (contact::id chat_id) const
{
    return _d->fetch_files(chat_id, _d->in_table_name);
}

static std::string const DELETE_BY_ID { "DELETE FROM \"{}\" WHERE file_id = {}" };

template <>
void file_cache_t::remove_outgoing_file (file::id file_id)
{
    debby::error err;
    auto sql = fmt::format(DELETE_BY_ID, _d->out_table_name, file_id);
    _d->pdb->query(sql, & err);

    if (err) {
        throw error {
              errc::storage_error
            , tr::f_("remove outgoing file failure: {}", file_id)
            , err.what()
        };
    }
}

template<>
void file_cache_t::remove_incoming_file (file::id file_id)
{
    debby::error err;
    auto sql = fmt::format(DELETE_BY_ID, _d->in_table_name, file_id);
    _d->pdb->query(sql, & err);

    if (err) {
        throw error {
              errc::storage_error
            , tr::f_("remove incoming file failure: {}", file_id)
            , err.what()
        };
    }
}

template <>
void file_cache_t::remove_broken ()
{
    static std::string const SELECT_PATH { "SELECT file_id, path FROM \"{}\"" };

    auto failure = _d->pdb->transaction([this] () {
        debby::error err;

        for (auto const * table_name: {& _d->in_table_name, & _d->out_table_name}) {
            auto res = _d->pdb->exec(fmt::format(SELECT_PATH, *table_name), & err);

            if (err)
                return pfs::make_optional(std::string{err.what()});

            if (res.has_more()) {
                auto file_id = res.get<file::id>("file_id", & err);
                auto path = res.get<std::string>("path", & err);

                if (err)
                    return pfs::make_optional(std::string{err.what()});

                if (file_id && path) {
                    auto x = fs::utf8_decode(*path);

                    if (!fs::exists(x)) {
                        auto sql = fmt::format(DELETE_BY_ID, *table_name, *file_id);
                        _d->pdb->query(sql, & err);

                        if (err)
                            return pfs::make_optional(std::string{err.what()});
                    }
                }
            }
        }

        return pfs::optional<std::string>{};
    });
}


template<>
void file_cache_t::clear ()
{
    static std::string const CLEAR_TABLE { "DELETE FROM \"{}\"" };

    debby::error err;

    for (auto const * table_name: {& _d->out_table_name, & _d->in_table_name}) {
        _d->pdb->query(fmt::format(CLEAR_TABLE, *table_name), & err);
    }

    if (err)
        throw error{errc::storage_error, err.what()};
}

} // namespace chat
