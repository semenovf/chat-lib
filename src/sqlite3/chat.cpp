////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.01.02 Initial version.
//      2022.02.17 Refactored totally.
//      2024.11.29 Started V2.
////////////////////////////////////////////////////////////////////////////////
#include "chat_impl.hpp"
#include "editor_impl.hpp"
#include "chat/chat.hpp"
#include "chat/editor_mode.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <pfs/debby/data_definition.hpp>
#include <pfs/debby/relational_database.hpp>
#include <array>

CHAT__NAMESPACE_BEGIN

using relational_database_t = debby::relational_database<debby::backend_enum::sqlite3>;
using data_definition_t = debby::data_definition<debby::backend_enum::sqlite3>;

namespace storage {

std::function<std::string ()> sqlite3::chat_table_name_prefix = [] { return std::string("#"); };
std::function<std::size_t ()> sqlite3::cache_window_size = [] { return std::size_t{100}; };

sqlite3::chat::chat (contact::id an_author_id, contact::id a_chat_id, relational_database_t & db)
    : author_id(an_author_id)
    , chat_id(a_chat_id)
{
    table_name = chat_table_name_prefix() + to_string(chat_id);

    debby::error err;

    if (!db.exists(table_name, & err)) {
        auto chat_table = data_definition_t::create_table(table_name);
        chat_table.add_column<decltype(message::message_credentials::message_id)>("message_id").unique();
        chat_table.add_column<decltype(message::message_credentials::author_id)>("author_id");
        chat_table.add_column<decltype(message::message_credentials::creation_time)>("creation_time");
        chat_table.add_column<decltype(message::message_credentials::modification_time)>("modification_time");
        chat_table.add_column<decltype(*message::message_credentials::delivered_time)>("delivered_time").nullable();
        chat_table.add_column<decltype(*message::message_credentials::read_time)>("read_time").nullable();
        chat_table.add_column<std::string>("content").nullable();

        std::array<std::string, 1> sqls = {
            chat_table.build()
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

        if (failure)
            throw error {errc::storage_error, failure.value()};
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    pdb = & db;
}

void sqlite3::chat::invalidate_cache ()
{
    cache.dirty = true;
}

void sqlite3::chat::prefetch (int offset, int limit, int sort_flags)
{
    static std::string const SELECT_ROWS_RANGE {
        "SELECT message_id, author_id, creation_time, modification_time, delivered_time, read_time"
        ", content FROM \"{}\" ORDER BY {} {} LIMIT {} OFFSET {}"
    };

    bool prefetch_required = cache.dirty
        || offset < cache.offset
        || offset + limit > cache.offset + cache.limit
        || sort_flags != cache.sort_flags;

    if (!prefetch_required)
        return;

    cache.data.clear();
    cache.map.clear();
    cache.offset = offset;
    cache.limit = 0;
    cache.dirty = true;
    cache.sort_flags = sort_flags;

    std::string field = "rowid";
    std::string order = "ASC";

    if (sort_flag_on(sort_flags, chat_sort_flag::by_id))
        field = "rowid";
    else if (sort_flag_on(sort_flags, chat_sort_flag::by_creation_time))
        field = "creation_time";
    else if (sort_flag_on(sort_flags, chat_sort_flag::by_modification_time))
        field = "modification_time";
    else if (sort_flag_on(sort_flags, chat_sort_flag::by_delivered_time))
        field = "delivered_time";
    else if (sort_flag_on(sort_flags, chat_sort_flag::by_read_time))
        field = "read_time";

    if (sort_flag_on(sort_flags, chat_sort_flag::ascending_order))
        order = "ASC";
    else if (sort_flag_on(sort_flags, chat_sort_flag::descending_order))
        order = "DESC";

    debby::error err;
    auto res = pdb->exec(fmt::format(SELECT_ROWS_RANGE, table_name, field, order
        , limit, offset), & err);

    if (!err) {
        for (; res.has_more(); res.next()) {
            cache.data.emplace_back();
            message::message_credentials * m = & cache.data.back();
            fill_message(res, *m);
            cache.map.emplace(m->message_id, cache.data.size() - 1);
            cache.limit++;
        }
    }

    cache.dirty = false;
}

void sqlite3::chat::fill_message (relational_database_t::result_type & result, message::message_credentials & m)
{
    m.message_id        = result.get_or("message_id", message::id{});
    m.author_id         = result.get_or("author_id", contact::id{});
    m.creation_time     = result.get_or("creation_time", pfs::utc_time_point{});
    m.modification_time = result.get_or("modification_time", pfs::utc_time_point{});
    m.delivered_time    = result.get_or("delivered_time", pfs::utc_time_point{});
    m.read_time         = result.get_or("read_time", pfs::utc_time_point{});
    auto content_data   = result.get<std::string>("content");

    if (content_data)
        m.contents = message::content {*content_data};
}

} // namespace storage

using chat_t = chat<storage::sqlite3>;

template <>
chat_t::chat () = default;

template <>
chat_t::chat (chat && other)
    : _d(std::move(other._d))
    , cache_outgoing_local_file(std::move(other.cache_outgoing_local_file))
    , cache_outgoing_custom_file(std::move(other.cache_outgoing_custom_file))
{
    other.cache_outgoing_local_file = nullptr;
    other.cache_outgoing_custom_file = nullptr;
}

template <>
chat_t & chat_t::operator = (chat && other)
{
    _d = std::move(other._d);
    cache_outgoing_local_file = std::move(other.cache_outgoing_local_file);
    cache_outgoing_custom_file = std::move(other.cache_outgoing_custom_file);
    other.cache_outgoing_local_file = nullptr;
    other.cache_outgoing_custom_file = nullptr;
    return *this;
}

template <>
chat_t::chat (rep * d) noexcept
    : _d(d)
{}

template <> chat_t::~chat () = default;

template <>
chat_t::operator bool () const noexcept
{
    return !!_d;
}

template <>
contact::id chat_t::id () const noexcept
{
    return _d->chat_id;
}

template <>
std::size_t chat_t::count () const
{
    return _d->pdb->rows_count(_d->table_name);
}

template <>
std::size_t chat_t::unread_message_count () const
{
    static std::string const UNREAD_MESSAGES_COUNT {
        "SELECT COUNT(1) as count FROM \"{}\" WHERE read_time IS NULL AND author_id != :author_id"
    };

    std::size_t count = 0;
    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(UNREAD_MESSAGES_COUNT, _d->table_name), & err);

    if (!err) {
        stmt.bind(":author_id", _d->author_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (!err) {
                if (res.has_more()) {
                    count = res.get_or(0, std::size_t{0});
                    res.next();
                }
            }
        }
    }

    if (err)
        throw error { errc::storage_error, tr::f_("get unread message count failure: {}", err.what()) };

    return count;
}

static void mark_message_status (relational_database_t * pdb
    , std::string const & sql
    , std::string const & table_name
    , message::id message_id
    , pfs::utc_time_point time
    , std::string const & status_str)
{
    debby::error err;
    auto stmt = pdb->prepare_cached(fmt::format(sql, table_name), & err);

    if (!err) {
        stmt.bind(":time", time, & err)
            && stmt.bind(":message_id", message_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (res.rows_affected() == 0) {
                throw error {
                    errc::message_not_found, tr::f_("no message mark {}: {}", status_str, message_id)
                };
            }
        }
    }

    if (err) {
        throw error {
              errc::storage_error, tr::f_("message ({}) mark as {} failure", message_id, status_str)
            , err.what()
        };
    }
}

template <>
void chat_t::mark_delivered (message::id message_id, pfs::utc_time_point delivered_time)
{
    static std::string const UPDATE_DELIVERED_TIME {
        "UPDATE OR IGNORE \"{}\" SET delivered_time = :time WHERE message_id = :message_id"
    };

    mark_message_status(_d->pdb, UPDATE_DELIVERED_TIME, _d->table_name, message_id, delivered_time, "delivered");
    _d->invalidate_cache();
}

template <>
void chat_t::mark_read (message::id message_id, pfs::utc_time_point read_time)
{
    static std::string const UPDATE_READ_TIME {
        "UPDATE OR IGNORE \"{}\" SET read_time = :time WHERE message_id = :message_id"
    };

    mark_message_status(_d->pdb, UPDATE_READ_TIME, _d->table_name, message_id, read_time, "read");
    _d->invalidate_cache();
}

template <>
chat_t::editor_type chat_t::create ()
{
    if (!this->_d)
        throw error {pfs::errc::null_pointer, tr::_("chat is null")};

    auto message_id = message::id_generator{}.next();
    editor_type ed {new storage::sqlite3::editor(& *this->_d, message_id, editor_mode::create)};
    ed.cache_outgoing_local_file = cache_outgoing_local_file;
    ed.cache_outgoing_custom_file = cache_outgoing_custom_file;
    return ed;
}


template <>
chat_t::editor_type chat_t::open (message::id message_id)
{
    static std::string const SELECT_OUTGOING_CONTENT {
        "SELECT message_id, content"
        " FROM \"{}\" WHERE message_id = :message_id AND author_id = :author_id"
    };       //                                                    ^
             //                                                    |
             // Interest is shown only to the outgouing message ----

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_OUTGOING_CONTENT, _d->table_name), & err);

    stmt.bind(":message_id", message_id, & err)
        && stmt.bind(":author_id", _d->author_id, & err);

    if (!err) {
        auto res = stmt.exec(& err);

        if (!err) {
            if (res.has_more()) {
                auto message_id = res.get_or("message_id", message::id{});
                auto content_data = res.get_or("content", std::string{});

                message::content content;

                if (!content_data.empty())
                    content = message::content{content_data};

                editor_type ed {new storage::sqlite3::editor(& *this->_d, message_id, std::move(content), editor_mode::modify)};
                ed.cache_outgoing_local_file = cache_outgoing_local_file;
                return ed;
            }
        }
    }

    if (err)
        throw error { errc::storage_error, tr::_("open editor failure"), err.what() };

    return editor_type{nullptr};
}

template <>
pfs::optional<message::message_credentials>
chat_t::message (int offset, int sort_flag) const
{
    bool force_populate_cache = _d->cache.dirty;

    // Out of range
    if (offset < _d->cache.offset || offset >= _d->cache.offset + _d->cache.limit)
        force_populate_cache = true;

    // Populate cache if dirty
    if (force_populate_cache)
        _d->prefetch(offset, storage::sqlite3::cache_window_size(), sort_flag);

    if (offset < _d->cache.offset || offset >= _d->cache.offset + _d->cache.limit)
        return pfs::nullopt;

    return _d->cache.data[offset - _d->cache.offset];
}

template <>
pfs::optional<message::message_credentials>
chat_t::message (message::id message_id) const
{
    static std::string const SELECT_MESSAGE {
        "SELECT message_id"
            ", author_id"
            ", creation_time"
            ", modification_time"
            ", delivered_time"
            ", read_time"
            ", content"
            " FROM \"{}\" WHERE message_id = :message_id"
    };

    // Check cache
    if (!_d->cache.dirty) {
        auto it = _d->cache.map.find(message_id);

        if (it != _d->cache.map.end()) {
            PFS__TERMINATE(it->second >= 0 && it->second < _d->cache.data.size()
                , "Unexpected condition");
            return _d->cache.data[it->second];
        }
    }

    debby::error err;
    auto stmt = _d->pdb->prepare_cached(fmt::format(SELECT_MESSAGE, _d->table_name), & err);

    if (!err) {
        stmt.bind(":message_id", message_id, & err);

        if (!err) {
            auto res = stmt.exec(& err);

            if (res.has_more()) {
                message::message_credentials m;
                _d->fill_message(res, m);
                return m;
            }
        }
    }

    if (err)
        throw error {errc::storage_error, err.what()};

    return pfs::nullopt;
}

template <>
pfs::optional<message::message_credentials>
chat_t::last_message () const
{
    static std::string const SELECT_LAST_MESSAGE {
        "SELECT message_id"
            ", author_id"
            ", creation_time"
            ", modification_time"
            ", delivered_time"
            ", read_time"
            ", content"
            " FROM \"{}\" ORDER BY ROWID DESC LIMIT 1"
    };

    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_LAST_MESSAGE, _d->table_name), & err);

    if (!err) {
        if (res.has_more()) {
            message::message_credentials m;
            _d->fill_message(res, m);
            return m;
        }
    }

    return pfs::nullopt;
}

template <>
void chat_t::save_incoming (message::id message_id, contact::id author_id
    , pfs::utc_time_point const & creation_time, std::string const & content)
{
    static std::string const INSERT_INCOMING_MESSAGE {
        "INSERT INTO \"{}\" (message_id, author_id, creation_time, modification_time, content)"
        " VALUES (:message_id, :author_id, :creation_time, :modification_time, :content)"
    };

    static std::string const UPDATE_INCOMING_MESSAGE {
        "UPDATE OR IGNORE \"{}\" SET creation_time = :time"
        ", modification_time = :time"
        ", content = :content"
        " WHERE message_id = :message_id"
    };

    auto m = message(message_id);
    debby::error err;

    // Message already exists
    if (m) {
        bool need_update = false;

        // Authors are different -> failure
        if (m->author_id != author_id) {
            auto err = error {
                  errc::inconsistent_data
                , tr::f_("authors are different: original {} and sender {}"
                    , m->author_id, author_id)
            };

            throw err;
        }

        // Content is different
        if (m->contents && !content.empty()) {
            if (m->contents->to_string() != content) {
                need_update = true;
            }
        }

        if (m->creation_time != creation_time)
            need_update = true;

        if (need_update) {
            auto stmt = _d->pdb->prepare_cached(fmt::format(UPDATE_INCOMING_MESSAGE, _d->table_name), & err);

            if (!err) {
                stmt.bind(":time", creation_time, & err)
                    && stmt.bind(":content", std::string{content}, & err)
                    && stmt.bind(":message_id", message_id, & err);

                if (!err) {
                    stmt.exec(& err);
                    _d->invalidate_cache();
                }
            }
        }
    } else {
        auto stmt = _d->pdb->prepare_cached(fmt::format(INSERT_INCOMING_MESSAGE, _d->table_name), & err);

        if (!err) {
            stmt.bind(":message_id", message_id, & err)
                && stmt.bind(":author_id", author_id, & err)
                && stmt.bind(":creation_time", creation_time, & err)
                && stmt.bind(":modification_time", creation_time, & err)
                && stmt.bind(":content", std::string{content}, & err);

            if (!err) {
                auto res = stmt.exec(& err);

                if (!err) {
                    if (res.rows_affected() == 0) {
                        throw error {
                              errc::inconsistent_data
                            , tr::f_("may be non-unique ID for incoming message: {}", message_id)
                        };
                    } else {
                        _d->invalidate_cache();
                    }
                }
            }
        }
    }
}

template <>
void chat_t::for_each (std::function<void(message::message_credentials const &)> f
    , int sort_flags, int max_count) const
{
    static std::string const SELECT_ALL_MESSAGES {
        "SELECT message_id"
            ", author_id"
            ", creation_time"
            ", modification_time"
            ", delivered_time"
            ", read_time"
            ", content"
            " FROM \"{}\" ORDER BY {} {}"
    };

    std::string field = "creation_time";
    std::string order = "ASC";

    if (sort_flag_on(sort_flags, chat_sort_flag::by_creation_time))
        field = "creation_time";
    else if (sort_flag_on(sort_flags, chat_sort_flag::by_modification_time))
        field = "modification_time";
    else if (sort_flag_on(sort_flags, chat_sort_flag::by_delivered_time))
        field = "delivered_time";
    else if (sort_flag_on(sort_flags, chat_sort_flag::by_read_time))
        field = "read_time";

    if (sort_flag_on(sort_flags, chat_sort_flag::ascending_order))
        order = "ASC";
    else if (sort_flag_on(sort_flags, chat_sort_flag::descending_order))
        order = "DESC";

    debby::error err;
    auto res = _d->pdb->exec(fmt::format(SELECT_ALL_MESSAGES, _d->table_name, field, order), & err);

    if (!err) {
        int counter = max_count < 0 ? -1 : max_count;

        while (res.has_more()) {
            if (max_count >= 0 && counter-- == 0)
                break;

            message::message_credentials m;

            m.message_id = res.get_or("message_id", message::id{});
            m.author_id = res.get_or("author_id", contact::id{});
            m.creation_time = res.get_or("creation_time", pfs::utc_time_point{});
            m.modification_time = res.get_or("modification_time", pfs::utc_time_point{});
            m.delivered_time = res.get<pfs::utc_time_point>("delivered_time");
            m.read_time = res.get<pfs::utc_time_point>("read_time");
            auto content_data = res.get_or("content", std::string{});

            if (!content_data.empty()) {
                message::content content{content_data};
                m.contents = std::move(content);
            }

            f(m);
            res.next();
        }
    }
}

template <>
void chat_t::clear ()
{
    _d->pdb->clear(_d->table_name);
    _d->invalidate_cache();
}

template <>
void chat_t::wipe ()
{
    _d->pdb->remove(_d->table_name);
    _d->invalidate_cache();
}

CHAT__NAMESPACE_END
