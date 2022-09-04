////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/chat/exports.hpp"
#include "pfs/chat/protocol.hpp"
#include <cereal/archives/portable_binary.hpp>
#include <sstream>

namespace chat {
namespace backend {
namespace cereal {

struct serializer
{
    using input_archive_type  = ::cereal::PortableBinaryInputArchive;
    using output_archive_type = ::cereal::PortableBinaryOutputArchive;

    class input_packet
    {
        std::istringstream _buf;
        input_archive_type _ar;

    public:
        input_packet (std::string const & data)
            : _buf(data)
            , _ar(_buf)
        {}

        input_packet (char const * data, std::streamsize size)
            : _ar(_buf)
        {
            _buf.rdbuf()->pubsetbuf(const_cast<char *>(data), size);
        }

        template <typename T>
        CHAT__EXPORT input_packet & operator >> (T & target);
    };

    class output_packet
    {
        std::ostringstream  _buf;
        output_archive_type _ar;

    public:
        output_packet () : _ar(_buf) {}

        template <typename T>
        CHAT__EXPORT output_packet & operator << (T const & payload);

        std::string data () const
        {
            return _buf.str();
        }
    };

    using input_packet_type  = input_packet;
    using output_packet_type = output_packet;
};

template <>
CHAT__EXPORT
serializer::input_packet &
serializer::input_packet::operator >> <protocol::packet_enum> (
    protocol::packet_enum & target);

template <>
CHAT__EXPORT
serializer::input_packet &
serializer::input_packet::operator >> <protocol::contact_credentials> (
    protocol::contact_credentials & target);

template <>
CHAT__EXPORT
serializer::output_packet &
serializer::output_packet::operator << <protocol::contact_credentials> (
    protocol::contact_credentials const & payload);

template <>
CHAT__EXPORT
serializer::input_packet &
serializer::input_packet::operator >> <protocol::regular_message> (
    protocol::regular_message & target);

template <>
CHAT__EXPORT
serializer::output_packet &
serializer::output_packet::operator << <protocol::regular_message> (
    protocol::regular_message const & payload);

template <>
CHAT__EXPORT
serializer::input_packet &
serializer::input_packet::operator >> <protocol::delivery_notification> (
    protocol::delivery_notification & target);

template <>
CHAT__EXPORT
serializer::output_packet &
serializer::output_packet::operator << <protocol::delivery_notification> (
    protocol::delivery_notification const & payload);

template <>
CHAT__EXPORT
serializer::input_packet &
serializer::input_packet::operator >> <protocol::read_notification> (
    protocol::read_notification & target);

template <>
CHAT__EXPORT
serializer::output_packet &
serializer::output_packet::operator << <protocol::read_notification> (
    protocol::read_notification const & payload);

// template <>
// CHAT__EXPORT
// serializer::input_packet &
// serializer::input_packet::operator >> <protocol::edited_message> (
//     protocol::edited_message & target);
//
// template <>
// CHAT__EXPORT
// serializer::output_packet &
// serializer::output_packet::operator << <protocol::edited_message> (
//     protocol::edited_message const & payload);

}}} // namespace chat::backend::cereal

