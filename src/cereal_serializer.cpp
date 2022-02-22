////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/chat/contact.hpp"
#include "pfs/chat/message.hpp"
#include "pfs/chat/protocol.hpp"
#include "pfs/chat/serializer.hpp"
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>
#include <sstream>

namespace cereal {

void save (cereal::PortableBinaryOutputArchive & ar, pfs::uuid_t const & uuid)
{
    ar << to_string(uuid);
}

void load (cereal::PortableBinaryInputArchive & ar, pfs::uuid_t & uuid)
{
    std::string s;
    ar >> s;
    uuid = pfs::from_string<pfs::uuid_t>(s);
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

////////////////////////////////////////////////////////////////////////////////
// original_message serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
std::string
serialize<protocol::original_message> (protocol::original_message const & m)
{
    std::ostringstream out;
    cereal::PortableBinaryOutputArchive ar{out};

    ar << static_cast<std::int8_t>(protocol::packet_type::original_message)
        << m.message_id
        << m.author_id
        << m.creation_time
        << m.content;

    return out.str();
}

template <>
protocol::original_message
deserialize<protocol::original_message> (std::string const & data)
{
    protocol::original_message m;
    std::istringstream in (data);
    cereal::PortableBinaryInputArchive ar(in);

    // Note: packet type must be read before
    ar >> m.message_id >> m.author_id >> m.creation_time >> m.content;

    return m;
}

////////////////////////////////////////////////////////////////////////////////
// delivery_notification serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
std::string
serialize<protocol::delivery_notification> (protocol::delivery_notification const & m)
{
    std::ostringstream out;
    cereal::PortableBinaryOutputArchive ar{out};

    ar << static_cast<std::int8_t>(protocol::packet_type::delivery_notification)
        << m.message_id
        << m.addressee_id
        << m.delivered_time;

    return out.str();
}

template <>
protocol::delivery_notification
deserialize<protocol::delivery_notification> (std::string const & data)
{
    protocol::delivery_notification m;
    std::istringstream in (data);
    cereal::PortableBinaryInputArchive ar(in);

    // Note: packet type must be read before
    ar >> m.message_id >> m.addressee_id >> m.delivered_time;

    return m;
}

////////////////////////////////////////////////////////////////////////////////
// read_notification serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
std::string
serialize<protocol::read_notification> (protocol::read_notification const & m)
{
    std::ostringstream out;
    cereal::PortableBinaryOutputArchive ar{out};

    ar << static_cast<std::int8_t>(protocol::packet_type::read_notification)
        << m.message_id
        << m.addressee_id
        << m.read_time;

    return out.str();
}

template <>
protocol::read_notification
deserialize<protocol::read_notification> (std::string const & data)
{
    protocol::read_notification m;
    std::istringstream in (data);
    cereal::PortableBinaryInputArchive ar(in);

    // Note: packet type must be read before
    ar >> m.message_id >> m.addressee_id >> m.read_time;

    return m;
}

////////////////////////////////////////////////////////////////////////////////
// edited_message serializer/deserializer
////////////////////////////////////////////////////////////////////////////////
template <>
std::string
serialize<protocol::edited_message> (protocol::edited_message const & m)
{
    std::ostringstream out;
    cereal::PortableBinaryOutputArchive ar{out};

    ar << static_cast<std::int8_t>(protocol::packet_type::edited_message)
        << m.message_id
        << m.author_id
        << m.modification_time
        << m.content;

    return out.str();
}

template <>
protocol::edited_message
deserialize<protocol::edited_message> (std::string const & data)
{
    protocol::edited_message m;
    std::istringstream in (data);
    cereal::PortableBinaryInputArchive ar(in);

    // Note: packet type must be read before
    ar >> m.message_id >> m.author_id >> m.modification_time >> m.content;

    return m;
}

} // chat
