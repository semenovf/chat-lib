////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "message_status.hpp"
#include "time_point.hpp"
#include "pfs/uuid.hpp"

namespace pfs {
namespace net {
namespace chat {

enum class message_item_type
{
      text = 1 // simple text message
    , file = 2 // file
};


// --------------------------------------------
// |                   message                |
// |------------------------------------------|
// |        |           content               |
// |        |---------------------------------|
// | header |    item     |    item     | ... |
// |        |-------------|-------------|-----|
// |        | text / file | text / file |     |
// --------------------------------------------

struct message_header
{
    uuid_t id;
    uuid_t author_id;
    time_point creation_time;
//     optional<timestamp_t> received_time;
//     optional<timestamp_t> read_time;
};

// struct message_item
// {
//     message_type type {message_type::text};
//     message_item * next {nullptr};
// };
//
// struct message_content
// {
//     message_item * first {nullptr};
//     message_item * last {nullptr};
// };
//
// struct message: message_header, message_content
// {};
//
// struct text_item: message_item
// {
//     std::string text;
// };
//
// struct file_item: message_item
// {
//     mime_type mime {mime_type::application__octet_stream};
//
//     // full path on sender side, and file name on receiver side
//     std::string filename;
//
//     // File size in bytes
//     std::size_t filesize;
// };
//
// template <typename T>
// std::unique_ptr<T> create ();
//
// template <>
// inline std::unique_ptr<message> create<message> ()
// {
//     using namespace std::chrono;
//
//     std::unique_ptr<message> result {new message};
//     result->id = chat::generate_uuid();
//     result->creation_time = std::chrono::system_clock::now();
//     return result;
// }
//
// template <>
// inline std::unique_ptr<text_item> create ()
// {
//     std::unique_ptr<text_item> result {new text_item};
//     result->type = message_type::text;
//     return result;
// }
//
// template <>
// inline std::unique_ptr<file_item> create ()
// {
//     std::unique_ptr<file_item> result {new file_item};
//     result->type = message_type::file;
//     return result;
// }
//
// PFS_CHAT_DLL_API void push (message & m, message_item * item_ptr);
//
// template <typename V>
// void for_each_mutable (message & m, V && visitor)
// {
//     for (message_item * first = m.first; first; first = first->next) {
//         visitor(first);
//     }
// }
//
// template <typename V>
// void for_each (message const & m, V && visitor)
// {
//     for (message_item const * first = m.first; first; first = first->next) {
//         visitor(first);
//     }
// }



// class content
// {
//
// };
//
// class message
// {
// #if PFS_HAVE_STD_OPTIONAL
//     using optional_content = std::optional<std::string>;
// #else
//     using optional_content = pfs::optional<std::string>;
// #endif
//
// public:
//     using id_type = std::uint32_t;
//
// private:
//     id_type _id;
//     uuid_t  _uuid;    // Author UUID
//     status  _state;
//     content _content; // Message content (text, reference(s) to media resources)
//
// public:
//     uuid_t uuid ()  const noexcept
//     {
//         return _uuid;
//     }
//
//     status state () const noexcept
//     {
//         return _state;
//     }
// };

}}} // namespace pfs::net::chat


