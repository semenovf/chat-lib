////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// References:
//
// Changelog:
//      2021.08.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef PFS_CHAT_STATIC_LIB
#   ifndef PFS_CHAT_DLL_API
#       if defined(_MSC_VER)
#           if defined(PFS_CHAT_DLL_EXPORTS)
#               define PFS_CHAT_DLL_API __declspec(dllexport)
#           else
#               define PFS_CHAT_DLL_API __declspec(dllimport)
#           endif
#       endif
#   endif
#endif // !PFS_CHAT_STATIC_LIB

#ifndef PFS_CHAT_DLL_API
#   define PFS_CHAT_DLL_API
#endif
