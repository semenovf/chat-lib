////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "pfs/optional.hpp"
#include "pfs/time_point.hpp"
#include "pfs/uuid.hpp"
#include "pfs/variant.hpp"
#include <memory>

namespace pfs {
namespace chat {
namespace message {

using message_id = uuid_t;

enum class route_enum {
      incoming = 1
    , outgoing = 2
};

enum class status_enum
{
      draft
    , dispatched
    , delivered
    , read
};

// Media Types
// [Media Types](https://www.iana.org/assignments/media-types/media-types.xhtml)
enum class mime_enum
{
    // The default value for textual files.
    // A textual file should be human-readable and must not contain binary data.
      text__plain

    , text__html

    // The default value for all other cases. An unknown file type should
    // use this type.
    //
    // Unrecognized subtypes of "audio" should at a miniumum be treated as
    // "application/octet-stream".
    // [RFC 2046](https://www.iana.org/assignments/media-types/audio/basic)
    , application__octet_stream
};

class id_generator
{
public:
    using type = message_id;

public:
    id_generator () {}

    type next () noexcept
    {
        return generate_uuid();
    }
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

struct item;
struct content;

struct credentials
{
    message_id id;                          // Unique message id
    bool deleted;                           // Deleted message flag
    contact::contact_id contact_id;         // Author (outgoing)/ addressee (incoming) contact ID
    utc_time_point creation_time;           // Message creation time (UTC)
    optional<utc_time_point> received_time; // Message received time (UTC)
    optional<utc_time_point> read_time;     // Message read time (UTC)
};

// struct message : header
// {
//     item * first {nullptr};
//     item * last {nullptr};
// };
//
// struct item
// {
//     item * next {nullptr};
//     std::unique_ptr<content> c;
// };
//
// using text_plain = std::string;
// using text_html  = std::string;

struct file_credentials
{
    message_id msg_id; // Message id
    bool deleted;          // Deleted message flag
    std::string name;      // File name (for incoming message)/path (for outgoing message)
    std::size_t size;      // File size
    std::string sha256;    // File SHA-256 checksum
};

// struct content
// {
//     mime_enum mime;
//     variant<
//           std::string
//         , file_spec    // full path on sender side, and file name on receiver side
//     > data;
// };

}}} // namespace pfs::chat::message
