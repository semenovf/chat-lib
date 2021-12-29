////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.08.14 Initial version.
//      2021.11.17 Refactored.
//      2021.11.29 Copied from `debby-lib` and fixed.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef CHAT__STATIC
#   ifndef CHAT__EXPORT
#       if _MSC_VER
#           if defined(CHAT__EXPORTS)
#               define CHAT__EXPORT __declspec(dllexport)
#           else
#               define CHAT__EXPORT __declspec(dllimport)
#           endif
#       else
#           define CHAT__EXPORT
#       endif
#   endif
#else
#   define CHAT__EXPORT
#endif // !CHAT__STATIC
