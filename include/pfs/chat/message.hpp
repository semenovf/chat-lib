////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
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

namespace chat {
namespace message {

using message_id = pfs::uuid_t;

// enum class status_enum
// {
//       draft
//     , dispatched
//     , delivered
//     , read
// };

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
        return pfs::generate_uuid();
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

struct incoming_credentials
{
    message_id id;                                // Unique message id (generated by author)
    contact::contact_id author_id;                // Author contact ID
    pfs::utc_time_point received_time;            // Message received time (UTC)
    pfs::optional<pfs::utc_time_point> read_time; // Message read time (UTC)
};

struct outgoing_credentials
{
    message_id id;                                    // Unique message id
    contact::contact_id addressee_id;                 // Addressee contact ID
    pfs::utc_time_point creation_time;                // Message creation time (UTC)
    pfs::optional<pfs::utc_time_point> sending_time;  // Message sending time (UTC)
    pfs::optional<pfs::utc_time_point> received_time; // Message received time (UTC)
    pfs::optional<pfs::utc_time_point> read_time;     // Message read time (UTC)
};

struct content_credentials
{
    mime_enum   mime;   // Message content MIME
    std::string text;   // Message content
};

struct file_credentials
{
    std::string name;      // File name (for incoming message)/path (for outgoing message)
    std::size_t size;      // File size
    std::string sha256;    // File SHA-256 checksum
};

}} // namespace chat::message
