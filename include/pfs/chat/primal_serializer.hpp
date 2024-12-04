////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2024.04.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "contact.hpp"
#include "message.hpp"
#include "protocol.hpp"
#include <pfs/endian.hpp>
#include <pfs/binary_istream.hpp>
#include <pfs/binary_ostream.hpp>
#include <pfs/numeric_cast.hpp>
#include <pfs/time_point_pack.hpp>
#include <pfs/universal_id_pack.hpp>

CHAT__NAMESPACE_BEGIN

template <pfs::endian Endianess = pfs::endian::network>
struct primal_serializer
{
    using output_archive_type = std::vector<char>;
    using input_archive_type  = pfs::string_view;
    using ostream_type = pfs::binary_ostream<Endianess>;
    using istream_type = pfs::binary_istream<Endianess>;

    ////////////////////////////////////////////////////////////////////////////////
    // message::content
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, message::content const & content)
    {
        out << to_string(content);
    }

    static void unpack (istream_type & in, message::content & content)
    {
        std::string source;
        in >> source;
        content = message::content{source};
    }

    ////////////////////////////////////////////////////////////////////////////////
    // protocol::packet_enum
    ////////////////////////////////////////////////////////////////////////////////
    static void unpack (istream_type & in, protocol::packet_enum & target)
    {
        in >> target;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // contact_credentials serializer/deserializer
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, protocol::contact_credentials const & payload)
    {
        out << protocol::packet_enum::contact_credentials
            << payload.contact.contact_id
            << payload.contact.creator_id
            << payload.contact.alias
            << payload.contact.avatar
            << payload.contact.description
            << payload.contact.extra
            << payload.contact.type;
    }

    static void unpack (istream_type & in, protocol::contact_credentials & target)
    {
        // Note: packet type must be read before
        in  >> target.contact.contact_id
            >> target.contact.creator_id
            >> target.contact.alias
            >> target.contact.avatar
            >> target.contact.description
            >> target.contact.extra
            >> target.contact.type;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // group_members serializer/deserializer
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, protocol::group_members const & payload)
    {
        out << protocol::packet_enum::group_members
            << payload.group_id
            << pfs::numeric_cast<typename ostream_type::size_type>(payload.members.size());

        for (auto const & x: payload.members)
            out << x;
    }

    static void unpack (istream_type & in, protocol::group_members & target)
    {
        typename ostream_type::size_type sz = 0;

        // Note: packet type must be read before
        in >> target.group_id >> sz;

        if (sz > 0) {
            target.members.reserve(sz);
            contact::id x;

            for (typename ostream_type::size_type i = 0; i < sz; i++) {
                in >> x;
                target.members.push_back(x);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // regular_message serializer/deserializer
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, protocol::regular_message const & payload)
    {
        out << protocol::packet_enum::regular_message
            << payload.message_id
            << payload.author_id
            << payload.chat_id
            << payload.mod_time
            << payload.content;
    }

    static void unpack (istream_type & in, protocol::regular_message & target)
    {
        // Note: packet type must be read before
        in  >> target.message_id
            >> target.author_id
            >> target.chat_id
            >> target.mod_time
            >> target.content;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // delivery_notification serializer/deserializer
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, protocol::delivery_notification const & payload)
    {
        out << protocol::packet_enum::delivery_notification
            << payload.message_id
            << payload.chat_id
            << payload.delivered_time;
    }

    static void unpack (istream_type & in, protocol::delivery_notification & target)
    {
        // Note: packet type must be read before
        in  >> target.message_id
            >> target.chat_id
            >> target.delivered_time;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // read_notification serializer/deserializer
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, protocol::read_notification const & payload)
    {
        out << protocol::packet_enum::read_notification
            << payload.message_id
            << payload.chat_id
            << payload.read_time;
    }

    static void unpack (istream_type & in, protocol::read_notification & target)
    {
        // Note: packet type must be read before
        in  >> target.message_id
            >> target.chat_id
            >> target.read_time;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // file_request serializer/deserializer
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, protocol::file_request const & payload)
    {
        out << protocol::packet_enum::file_request << payload.file_id;
    }

    static void unpack (istream_type & in, protocol::file_request & target)
    {
        // Note: packet type must be read before
        in >> target.file_id;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // file_error serializer/deserializer
    ////////////////////////////////////////////////////////////////////////////////
    static void pack (ostream_type & out, protocol::file_error const & payload)
    {
        out << protocol::packet_enum::file_error << payload.file_id;
    }

    static void unpack (istream_type & in, protocol::file_error & target)
    {
        // Note: packet type must be read before
        in >> target.file_id;
    }
};

namespace message {

template <typename Packet, pfs::endian Endianess>
inline void pack (pfs::binary_ostream<Endianess> & out, Packet const & pkt)
{
    primal_serializer<Endianess>::pack(out, pkt);
}

template <typename Packet, pfs::endian Endianess>
inline void unpack (pfs::binary_istream<Endianess> & in, Packet & pkt)
{
    primal_serializer<Endianess>::unpack(in, pkt);
}

} // namespace message

namespace protocol {

template <typename Packet, pfs::endian Endianess>
inline void pack (pfs::binary_ostream<Endianess> & out, Packet const & pkt)
{
    primal_serializer<Endianess>::pack(out, pkt);
}

template <typename Packet, pfs::endian Endianess>
inline void unpack (pfs::binary_istream<Endianess> & in, Packet & pkt)
{
    primal_serializer<Endianess>::unpack(in, pkt);
}

} // namespace protocol

CHAT__NAMESPACE_END
