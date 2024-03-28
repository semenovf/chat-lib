////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.21 Initial version.
//      2022.03.17 Implementing input_packet and output_packet.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/backend/serializer/cereal.hpp"
#include "pfs/chat/contact.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/serializer.hpp"
#include "pfs/chat/protocol.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

namespace cereal {

using chat::protocol::packet_enum;

void save (cereal::PortableBinaryOutputArchive & ar, pfs::universal_id const & uuid)
{
    ar << to_string(uuid);
}

void load (cereal::PortableBinaryInputArchive & ar, pfs::universal_id & uuid)
{
    std::string s;
    ar >> s;
    uuid = pfs::from_string<pfs::universal_id>(s);
}

void save (cereal::PortableBinaryOutputArchive & ar, pfs::utc_time_point const & t)
{
    // Milliseconds is equivalent to integer type of at least 45 bits,
    // so nearest standard integer type is std::int64_t
    std::int64_t ticks = t.to_millis().count();
    ar << ticks;
}

void load (cereal::PortableBinaryInputArchive & ar, pfs::utc_time_point & t)
{
    std::int64_t ticks;
    ar >> ticks;
    t = pfs::utc_time_point{std::chrono::milliseconds{
        static_cast<std::chrono::milliseconds::rep>(ticks)}};
}

void save (cereal::PortableBinaryOutputArchive & ar, chat::message::content const & content)
{
    ar << to_string(content);
}

void load (cereal::PortableBinaryInputArchive & ar, chat::message::content & content)
{
    std::string source;
    ar >> source;
    content = chat::message::content{source};
}

} // namespace cereal

namespace chat {

namespace backend {
namespace cereal {

template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::packet_enum> (
    protocol::packet_enum & target)
{
    _ar >> target;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// contact_credentials serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::contact_credentials> (
    protocol::contact_credentials & target)
{
    // Note: packet type must be read before
    _ar >> target.contact.contact_id
        >> target.contact.creator_id
        >> target.contact.alias
        >> target.contact.avatar
        >> target.contact.description
        >> target.contact.extra
        >> target.contact.type;

    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::contact_credentials> (
    protocol::contact_credentials const & payload)
{
    _ar << protocol::packet_enum::contact_credentials
        << payload.contact.contact_id
        << payload.contact.creator_id
        << payload.contact.alias
        << payload.contact.avatar
        << payload.contact.description
        << payload.contact.extra
        << payload.contact.type;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// group_members serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::group_members> (
    protocol::group_members & target)
{
    // Note: packet type must be read before
    _ar >> target.group_id >> target.members;
    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::group_members> (
    protocol::group_members const & payload)
{
    _ar << protocol::packet_enum::group_members
        << payload.group_id
        << payload.members;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// regular_message serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::regular_message> (
    protocol::regular_message & target)
{
    // Note: packet type must be read before
    _ar >> target.message_id
        >> target.author_id
        >> target.conversation_id
        >> target.mod_time
        >> target.content;
    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::regular_message> (
    protocol::regular_message const & payload)
{
    _ar << protocol::packet_enum::regular_message
        << payload.message_id
        << payload.author_id
        << payload.conversation_id
        << payload.mod_time
        << payload.content;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// delivery_notification serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::delivery_notification> (
    protocol::delivery_notification & target)
{
    // Note: packet type must be read before
    _ar >> target.message_id
        >> target.conversation_id
        >> target.delivered_time;
    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::delivery_notification> (
    protocol::delivery_notification const & payload)
{
    _ar << protocol::packet_enum::delivery_notification
        << payload.message_id
        << payload.conversation_id
        << payload.delivered_time;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// read_notification serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::read_notification> (
    protocol::read_notification & target)
{
    // Note: packet type must be read before
    _ar >> target.message_id
        >> target.conversation_id
        >> target.read_time;

    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::read_notification> (
    protocol::read_notification const & payload)
{
    _ar << protocol::packet_enum::read_notification
        << payload.message_id
        << payload.conversation_id
        << payload.read_time;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// file_request serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::file_request> (
    protocol::file_request & target)
{
    // Note: packet type must be read before
    _ar >> target.file_id;

    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::file_request> (
    protocol::file_request const & payload)
{
    _ar << protocol::packet_enum::file_request
        << payload.file_id;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// file_error serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::file_error> (
    protocol::file_error & target)
{
    // Note: packet type must be read before
    _ar >> target.file_id;

    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::file_error> (
    protocol::file_error const & payload)
{
    _ar << protocol::packet_enum::file_error
        << payload.file_id;
    return *this;
}

}} // namespace backend::cereal

} // namespace chat
