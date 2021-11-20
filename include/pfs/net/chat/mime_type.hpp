////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace pfs {
namespace net {
namespace chat {

enum class mime_type
{
      // The default value for all other cases. An unknown file type should
      // use this type.
      application__octet_stream

      // The default value for textual files.
      // A textual file should be human-readable and must not contain binary data.
    , text__plain
};

}}} // namespace pfs::net::chat


