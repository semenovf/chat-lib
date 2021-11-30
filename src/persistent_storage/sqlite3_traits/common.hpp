////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.11.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/fmt.hpp"
#include "pfs/debby/basic_result.hpp"
#include <string>

namespace pfs {
namespace net {
namespace chat {
namespace sqlite3 {

template <typename NativeType>
std::string field_type ();

template <typename NativeType>
struct storage_type
{
    using type = void;
};

template <typename NativeType>
inline typename storage_type<NativeType>::type encode (NativeType const & orig)
{
    static_assert(std::is_same<typename storage_type<NativeType>::type, NativeType>::value
        , "Storage and native types are expected to be the same");
    return orig;
}

template <typename NativeType>
inline bool decode (typename storage_type<NativeType>::type const & orig, NativeType * target)
{
    static_assert(std::is_same<typename storage_type<NativeType>::type, NativeType>::value
        , "Storage and native types are expected to be the same");
    *target = orig;
    return true;
}

template <typename ResultImpl
    , typename NativeType
    , typename FailureCallback>
bool pull (debby::basic_result<ResultImpl> & res
    , std::string const & column_name
    , NativeType * target
    , FailureCallback failure_callback)
{
    using storage_type = typename storage_type<NativeType>::type;

    std::pair<storage_type, bool> x = res.template get<storage_type>(column_name
        , storage_type{});

    bool success = x.second;

    if (success)
        success = decode<NativeType>(x.first, target);

    if (!success)
        failure_callback(fmt::format("bad `{}` value", column_name));

    return success;
}

}}}} // namespace pfs::net::chat::sqlite3
