////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/time_point.hpp"
#include "pfs/uuid.hpp"
#include "pfs/variant.hpp"

namespace pfs {
namespace chat {
namespace message {

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
    using type = uuid_t;

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

struct header
{
    uuid_t id;
    uuid_t author_id;
    utc_time_point creation_time;
//     optional<timestamp_t> received_time;
//     optional<timestamp_t> read_time;
    item * first {nullptr};
    item * last {nullptr};
};

struct item
{
    item * next {nullptr};
    content * content;
};

using text_plain = std::string;
using text_html  = std::string;
using file_spec  = std::pair<std::string, std::size_t>;

struct content
{
    mime_enum mime;
    variant<
          std::string
        , file_spec    // full path on sender side, and file name on receiver side
    > data;
};

}}} // namespace pfs::chat::message
