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
namespace chat {
namespace sqlite3 {

template <typename NativeType>
struct field_type
{
    static std::string s ();
};

template <typename NativeType>
struct field_type<optional<NativeType>>
{
    static std::string s () { return field_type<NativeType>::s(); }
};

template <typename NativeType>
struct storage_type
{
    using type = void;
};

template <typename NativeType>
struct storage_type<optional<NativeType>>
{
    using type = typename storage_type<NativeType>::type;
};

template <typename NativeType>
struct codec
{
    static typename storage_type<NativeType>::type encode (NativeType const & orig)
    {
        static_assert(std::is_same<typename storage_type<NativeType>::type, NativeType>::value
            , "Storage and native types are expected to be the same");
        return orig;
    }

    static bool decode (typename storage_type<NativeType>::type const & orig, NativeType * target)
    {
        static_assert(std::is_same<typename storage_type<NativeType>::type, NativeType>::value
            , "Storage and native types are expected to be the same");
        *target = orig;
        return true;
    }
};

template <typename NativeType>
inline typename storage_type<NativeType>::type encode (NativeType const & orig)
{
    return codec<NativeType>::encode(orig);
}

template <typename NativeType>
inline bool decode (typename storage_type<NativeType>::type const & orig, NativeType * target)
{
    return codec<NativeType>::decode(orig, target);
}

template <typename ResultImpl
    , typename NativeType
    , typename FailureCallback>
struct puller
{
    static bool pull (debby::basic_result<ResultImpl> & res
        , std::string const & column_name
        , NativeType * target
        , FailureCallback failure_callback)
    {
        using storage_type = typename storage_type<NativeType>::type;

        auto expected_value = res.template get<storage_type>(column_name);
        bool success = !!expected_value;

        if (success)
            success = decode<NativeType>(*expected_value, target);

        if (!success)
            failure_callback(fmt::format("bad `{}` value", column_name));

        return success;
    }
};

template <typename ResultImpl
    , typename NativeType
    , typename FailureCallback>
struct puller<ResultImpl, optional<NativeType>, FailureCallback>
{
    static bool pull (debby::basic_result<ResultImpl> & res
        , std::string const & column_name
        , optional<NativeType> * target
        , FailureCallback failure_callback)
    {
        using storage_type = typename storage_type<NativeType>::type;

        auto expected_value = res.template get<storage_type>(column_name);
        bool success = !!expected_value;

        if (success) {
            NativeType t;
            success = decode<NativeType>(* expected_value, & t);

            if (success)
                *target = std::move(t);
        } else {
            // Column contains `null` value
            if (!expected_value.error()) {
                *target = nullopt;
                success = true;
            }
        }

        if (!success)
           failure_callback(fmt::format("bad `{}` value", column_name));

        return success;
    }
};

template <typename ResultImpl
    , typename NativeType
    , typename FailureCallback>
inline bool pull (debby::basic_result<ResultImpl> & res
    , std::string const & column_name
    , NativeType * target
    , FailureCallback failure_callback)
{
    return puller<ResultImpl, NativeType, FailureCallback>::pull(res
        , column_name
        , target
        , failure_callback);
}

}}} // namespace pfs::chat::sqlite3
