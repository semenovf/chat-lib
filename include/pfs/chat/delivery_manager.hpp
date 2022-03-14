////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.02.15 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "error.hpp"
#include "message.hpp"
#include "protocol.hpp"
#include "serializer.hpp"
#include "pfs/memory.hpp"
#include <functional>
#include <vector>

namespace chat {

//   Author                                               Addressee
// ---------                                              ---------
//     |                                                      |
//     |          1. message ID                               |
//     |          2. author ID                                |
//     |          3. creation time                            |
//     | (1)      4. content                             (1') |
//     |----------------------------------------------------->|
//     |                                                      |
//     |          1. message ID                               |
//     |          2. addressee ID                             |
//     | (2')     3. delivered time                       (2) |
//     |<-----------------------------------------------------|
//     |                                                      |
//     |          1. message ID                               |
//     |          2. addressee ID                             |
//     | (3')     3. read time                            (3) |
//     |<-----------------------------------------------------|
//     |                                                      |
//     |          1. message ID                               |
//     |          2. author ID                                |
//     |          3. modification time                        |
//     | (4)      4. content                             (4') |
//     |----------------------------------------------------->|
//     |                                                      |
//     |                         ...                          |
//     |                                                      |
//     | (4)                                             (4') |
//     |----------------------------------------------------->|
//     |                                                      |
//
// Step (1-1') - dispatching message
// Step (2-2') - delivery notification
// Step (3-3') - read notification
// Step (4-4') - modification notification
//
// Step (4-4') can happen zero or more times.
//

template <typename Backend>
class delivery_manager final
{
    using rep_type = typename Backend::rep;

public:
    using message_dispatched_callback = std::function<void(contact::contact_id
                , message::message_id
                , pfs::utc_time_point)>;
    using message_delivered_callback = message_dispatched_callback;
    using message_read_callback = message_dispatched_callback;

private:
    rep_type _rep;
//     std::map<message::message_id, std::time_point> dispatch_pending;
//     std::map<message::message_id, std::time_point> delivered_pending;
//     std::map<message::message_id, std::time_point> read_pending;

private:
    delivery_manager () = default;
    delivery_manager (rep_type && rep);
    delivery_manager (delivery_manager const & other) = delete;
    delivery_manager & operator = (delivery_manager const & other) = delete;
    delivery_manager & operator = (delivery_manager && other) = delete;

public:
    delivery_manager (delivery_manager && other) = default;
    ~delivery_manager () = default;

public:
    /**
     * Checks if delivery manager ready for use.
     */
    operator bool () const noexcept;

    /**
     * Dispatch original message.
     *
     * @return Serialized message or empty if error occured.
     */
    result_status dispatch (contact::contact_id addressee
        , message::message_credentials const & msg
        , message_dispatched_callback message_dispatched
        , message_delivered_callback message_delivered
        , message_read_callback message_read)
    {
        protocol::original_message m;
        m.message_id    = msg.id;
        m.author_id     = msg.author_id;
        m.creation_time = msg.creation_time;
        m.content       = msg.contents.has_value() ? to_string(*msg.contents) : std::string{};

        auto data = serialize<protocol::original_message>(m);
        auto res = _rep.send_message(addressee, msg.id, data
            , message_dispatched
            , message_delivered
            , message_read);

        return res;
    }

public:
    template <typename ...Args>
    static delivery_manager make (Args &&... args)
    {
        return delivery_manager{Backend::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<delivery_manager> make_unique (Args &&... args)
    {
        auto ptr = new delivery_manager{Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<delivery_manager>(ptr);
    }
};

} // namespace chat
