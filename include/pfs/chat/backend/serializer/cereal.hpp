////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.17 Initial version.
//      2022.11.11 `input_packet` reimplemented.
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
        struct buffer : public std::streambuf
        {
            buffer (char const * s, std::size_t n)
            {
                auto * p = const_cast<char *>(s);
                setg(p, p, p + n);
            }
        };

    private:
        buffer _buf;
        std::istream _archiver_backend;
        input_archive_type _ar;

    public:
        input_packet (std::string & data)
            : _buf(data.data(), data.size())
            , _archiver_backend(& _buf)
            , _ar(_archiver_backend)
        {}

        input_packet (char const * data, std::size_t size)
            : _buf(data, size)
            , _archiver_backend(& _buf)
            , _ar(_archiver_backend)
        {}

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
serializer::input_packet::operator >> <protocol::group_members> (
    protocol::group_members & target);

template <>
CHAT__EXPORT
serializer::output_packet &
serializer::output_packet::operator << <protocol::group_members> (
    protocol::group_members const & payload);

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

template <>
CHAT__EXPORT
serializer::input_packet &
serializer::input_packet::operator >> <protocol::file_request> (
    protocol::file_request & target);

template <>
CHAT__EXPORT
serializer::output_packet &
serializer::output_packet::operator << <protocol::file_request> (
    protocol::file_request const & payload);

template <>
CHAT__EXPORT
serializer::input_packet &
serializer::input_packet::operator >> <protocol::file_error> (
    protocol::file_error & target);

template <>
CHAT__EXPORT
serializer::output_packet &
serializer::output_packet::operator << <protocol::file_error> (
    protocol::file_error const & payload);

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

