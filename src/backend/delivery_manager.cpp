////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.18 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/delivery_manager.hpp"
#include "pfs/chat/backend/delivery_manager.hpp"

namespace chat {

namespace backend {

delivery_manager::rep_type
delivery_manager::make (error *)
{
    rep_type rep;
    return rep;
}

} // namespace backend

#define BACKEND backend::delivery_manager

template <>
delivery_manager<BACKEND>::delivery_manager (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
delivery_manager<BACKEND>::operator bool () const noexcept
{
    return true;
}

} // namespace chat
