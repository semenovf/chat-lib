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
#include <cereal/types/string.hpp>

namespace cereal {

using chat::protocol::packet_type_enum;

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
    std::int64_t ticks = to_millis(t).count();
    ar << ticks;
}

void load (cereal::PortableBinaryInputArchive & ar, pfs::utc_time_point & t)
{
    std::int64_t ticks;
    ar >> ticks;
    t = pfs::utc_time_point{pfs::from_millis(std::chrono::milliseconds{
        static_cast<std::chrono::milliseconds::rep>(ticks)})};
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

// void save (cereal::PortableBinaryOutputArchive & ar, packet_type_enum const & packet_type)
// {
//     ar << static_cast<std::underlying_type<packet_type_enum>
//         ::type>(packet_type);
// }

// void load (cereal::PortableBinaryInputArchive & ar, packet_type_enum & packet_type)
// {
//     std::underlying_type<packet_type_enum>::type pt;
//     ar >> pt;
//
// #define PACKET_TYPE_CAST(x)                                          \
//         static_cast<std::underlying_type<packet_type_enum>::type>(x)
//
//     switch (pt) {
//         case PACKET_TYPE_CAST(packet_type_enum::original_message):
//             packet_type = packet_type_enum::original_message;
//             break;
//         case PACKET_TYPE_CAST(packet_type_enum::delivery_notification):
//             packet_type = packet_type_enum::delivery_notification;
//             break;
//         case PACKET_TYPE_CAST(packet_type_enum::read_notification):
//             packet_type = packet_type_enum::read_notification;
//             break;
//         case PACKET_TYPE_CAST(packet_type_enum::edited_message):
//             packet_type = packet_type_enum::edited_message;
//             break;
//         default:
//             packet_type = packet_type_enum::unknown_packet;
//             break;
//     }
// }

// template <typename T>
// void save (cereal::PortableBinaryOutputArchive & ar, pfs::optional<T> const & opt)
// {
//     if (opt.has_value())
//         ar << std::int8_t{1} << *opt;
//     else
//         ar << std::int8_t{0};
// }
//
// template <typename T>
// void load (cereal::PortableBinaryInputArchive & ar, pfs::optional<T> & opt)
// {
//     std::int8_t has_value;
//     ar >> has_value;
//
//     if (has_value) {
//         T x;
//         ar >> x;
//         opt = std::move(x);
//     } else {
//         opt = pfs::nullopt;
//     }
// }

} // namespace cereal

namespace chat {

namespace backend {
namespace cereal {

template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::packet_type_enum> (
    protocol::packet_type_enum & target)
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
    _ar >> target.contact.id
        >> target.contact.creator_id
        >> target.contact.alias
        >> target.contact.avatar
        >> target.contact.description
        >> target.contact.type;

    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::contact_credentials> (
    protocol::contact_credentials const & payload)
{
    _ar << protocol::packet_type_enum::contact_credentials
        << payload.contact.id
        << payload.contact.creator_id
        << payload.contact.alias
        << payload.contact.avatar
        << payload.contact.description
        << payload.contact.type;

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// original_message serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::original_message> (
    protocol::original_message & target)
{
    // Note: packet type must be read before
    _ar >> target.message_id
        >> target.author_id
        >> target.creation_time
        >> target.content;
    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::original_message> (
    protocol::original_message const & payload)
{
    _ar << protocol::packet_type_enum::original_message
        << payload.message_id
        << payload.author_id
        << payload.creation_time
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
        >> target.addressee_id
        >> target.delivered_time;
    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::delivery_notification> (
    protocol::delivery_notification const & payload)
{
    _ar << protocol::packet_type_enum::delivery_notification
        << payload.message_id
        << payload.addressee_id
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
        >> target.addressee_id
        >> target.read_time;

    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::read_notification> (
    protocol::read_notification const & payload)
{
    _ar << protocol::packet_type_enum::read_notification
        << payload.message_id
        << payload.addressee_id
        << payload.read_time;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// edited_message serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
serializer::input_packet &
serializer::input_packet::operator >> <protocol::edited_message> (
    protocol::edited_message & target)
{
    // Note: packet type must be read before
    _ar >> target.message_id
        >> target.author_id
        >> target.modification_time
        >> target.content;

    return *this;
}

template <>
serializer::output_packet &
serializer::output_packet::operator << <protocol::edited_message> (
    protocol::edited_message const & payload)
{
    _ar << protocol::packet_type_enum::edited_message
        << payload.message_id
        << payload.author_id
        << payload.modification_time
        << payload.content;

    return *this;
}

}} // namespace backend::cereal

//using BACKEND = backend::cereal::serializer;

} // namespace chat
