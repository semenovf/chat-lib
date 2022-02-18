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
#include "pfs/memory.hpp"
#include <vector>

namespace chat {

//   Author                                               Addressee
// ---------                                              ---------
//     |                                                      |
//     |          1. author ID                                |
//     |          2. message ID                               |
//     |          3. creation time                            |
//     |          4. modification time                        |
//     |          5. dispatched time                          |
//     | (1)      6. content                             (1') |
//     |----------------------------------------------------->|
//     |                                                      |
//     |          1. addressee ID                             |
//     |          2. message ID                               |
//     | (2')     3. delivered time                       (2) |
//     |<-----------------------------------------------------|
//     |                                                      |
//     |          1. addressee ID                             |
//     |          2. message ID                               |
//     | (3')     3. read time                            (3) |
//     |<-----------------------------------------------------|
//     |                                                      |
//     |          1. author ID                                |
//     |          2. message ID                               |
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
    using rep_type = typename Backend::rep_type;

private:
    rep_type _rep;

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
     * Checks if delivery_manager ready for use.
     */
    operator bool () const noexcept;

    //bool dispatch (message::message_credentials const & msg, error * perr = nullptr);

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
