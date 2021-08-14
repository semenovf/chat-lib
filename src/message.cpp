////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/message.hpp"

namespace pfs {
namespace chat {

PFS_CHAT_DLL_API void push (message & m, message_item * item_ptr)
{
    item_ptr->next = nullptr;

    if (!m.last) {
        m.first = item_ptr;
        m.last = item_ptr;
    } else {
        m.last->next = item_ptr;
        m.last = item_ptr;
    }
}

}} // namespace pfs::chat

