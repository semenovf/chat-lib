////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include "types.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"
#include <memory>

namespace pfs {
namespace chat {

enum class message_type
{
      text // simple text message
    , file // file
};

enum class mime_type
{
      // The default value for all other cases. An unknown file type should
      // use this type.
      application__octet_stream

      // The default value for textual files.
      // A textual file should be human-readable and must not contain binary data.
    , text__plain
};

// --------------------------------------------
// |                   message                |
// |-------------------------------------------
// |        |    item     |    item     | ... |
// | header |-------------|-------------|-----|
// |        | text / file | text / file |     |
// --------------------------------------------

struct message_header
{
    uuid_t id;
    uuid_t sender_id;
    uuid_t receiver_id;
    timestamp_t creation_time;
    optional<timestamp_t> received_time;
    optional<timestamp_t> read_time;
};

struct message_item
{
    message_type type {message_type::text};
    message_item * next {nullptr};
};

struct message_content
{
    message_item * first {nullptr};
    message_item * last {nullptr};
};

struct message: message_header, message_content
{};

struct text_item: message_item
{
    std::string text;
};

struct file_item: message_item
{
    mime_type mime {mime_type::application__octet_stream};

    // full path on sender side, and file name on receiver side
    std::string filename;

    // File size in bytes
    std::size_t filesize;
};

template <typename T>
std::unique_ptr<T> create ();

template <>
inline std::unique_ptr<message> create<message> ()
{
    using namespace std::chrono;

    std::unique_ptr<message> result {new message};
    result->id = chat::generate_uuid();
    result->creation_time = std::chrono::system_clock::now();
    return result;
}

template <>
inline std::unique_ptr<text_item> create ()
{
    std::unique_ptr<text_item> result {new text_item};
    result->type = message_type::text;
    return result;
}

template <>
inline std::unique_ptr<file_item> create ()
{
    std::unique_ptr<file_item> result {new file_item};
    result->type = message_type::file;
    return result;
}

PFS_CHAT_DLL_API void push (message & m, message_item * item_ptr);

template <typename V>
void for_each_mutable (message & m, V && visitor)
{
    for (message_item * first = m.first; first; first = first->next) {
        visitor(first);
    }
}

template <typename V>
void for_each (message const & m, V && visitor)
{
    for (message_item const * first = m.first; first; first = first->next) {
        visitor(first);
    }
}

}} // namespace pfs::chat
