////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
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
        input_packet & operator >> (T & target);
    };

    class output_packet
    {
        std::ostringstream  _buf;
        output_archive_type _ar;

    public:
        output_packet () : _ar(_buf) {}

        template <typename T>
        output_packet & operator << (T const & payload);

        std::string data () const
        {
            return _buf.str();
        }
    };

    using input_packet_type   = input_packet;
    using output_packet_type  = output_packet;
};

}}} // namespace chat::backend::cereal

