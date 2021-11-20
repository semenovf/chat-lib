////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
//      2021.11.17 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef PFS_CHAT_LIB_STATIC
#   ifndef PFS_CHAT_LIB_DLL_EXPORT
#       if _MSC_VER
#           if defined(PFS_CHAT_LIB_EXPORTS)
#               define PFS_CHAT_LIB_DLL_EXPORT __declspec(dllexport)
#           else
#               define PFS_CHAT_LIB_DLL_EXPORT __declspec(dllimport)
#           endif
#       else
#           define PFS_CHAT_LIB_DLL_EXPORT
#       endif
#   endif
#else
#   define PFS_CHAT_LIB_DLL_EXPORT
#endif // !PFS_NET_LIB_STATIC
