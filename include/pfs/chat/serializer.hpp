////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.03.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace chat {

template <typename Backend>
class serializer
{
public:
    using input_packet_type  = typename Backend::input_packet_type;
    using output_packet_type = typename Backend::output_packet_type;

private:
    serializer () = delete;
    serializer (serializer const & other) = delete;
    serializer & operator = (serializer const & other) = delete;
    serializer & operator = (serializer && other) = delete;
    serializer (serializer && other) = delete;
    ~serializer () = delete;
};

} // namespace chat
