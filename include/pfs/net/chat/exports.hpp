////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
//      2021.11.17 Refactored.
//      2021.11.29 Copied from `debby-lib` and fixed.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef PFS_CHAT__STATIC
#   ifndef PFS_CHAT__EXPORT
#       if _MSC_VER
#           if defined(PFS_CHAT__EXPORTS)
#               define PFS_CHAT__EXPORT __declspec(dllexport)
#           else
#               define PFS_CHAT__EXPORT __declspec(dllimport)
#           endif
#       else
#           define PFS_CHAT__EXPORT
#       endif
#   endif
#else
#   define PFS_CHAT__EXPORT
#endif // !PFS_CHAT__STATIC
