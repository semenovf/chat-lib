////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.11.03 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "chat/contact.hpp"
#include "chat/file.hpp"
#include <string>
#include <vector>

namespace chat {

class function_callbacks
{
public:
    /**
     * Called to dispatch data (pass to delivery manager).
     *
     * @param message_address Message address.
     */
    mutable std::function<void (contact::id /*addressee*/
        , std::vector<char> const & /*data*/)> dispatch_data
    = [] (contact::id, std::vector<char> const &) {};

    /**
     * Called when file/attachment request received.
     */
    mutable std::function<void (contact::id /*addressee_id*/
        , file::id /*file_id*/
        , std::string const & /*path*/)> dispatch_file
    = [] (contact::id, file::id, std::string const &) {};

    /**
     * Called by receiver when message received.
     *
     * @param author_id Author/sender identifier.
     * @param conversation_id Conversation identifier.
     * @param message_id Message identifier.
     */
    mutable std::function<void (contact::id /*author_id*/
        , contact::id /*conversation_id*/
        , message::id /*message_id*/)> message_received
    = [] (contact::id, contact::id, message::id) {};

    /**
     * Called by author when message delivered to addressee (receiver).
     *
     * @param conversation_id Conversation identifier.
     * @param message_id Message identifier.
     * @param delivered_time Delivered time in UTC.
     */
    mutable std::function<void (contact::id /*conversation_id*/
        , message::id /*message_id*/
        , pfs::utc_time_point /*delivered_time*/)> message_delivered
    = [] (contact::id, message::id, pfs::utc_time_point) {};

    /**
     * Called by author when received read message notification or opponent when
     * read received message from author.
     *
     * @param conversation_id Conversation identifier.
     * @param message_id Message identifier.
     * @param delivered_time Read time in UTC.
     */
    mutable std::function<void (contact::id /*conversation_id*/
        , message::id /*message_id*/
        , pfs::utc_time_point /*read_time*/)> message_read
    = [] (contact::id, message::id, pfs::utc_time_point) {};

    /**
     * Called after adding contact.
     */
    mutable std::function<void (contact::id)> contact_added
    = [] (contact::id) {};

    /**
     * Called after updating contact.
     */
    mutable std::function<void (contact::id)> contact_updated
    = [] (contact::id) {};

    /**
     * Called after contact removed.
     */
    mutable std::function<void (contact::id)> contact_removed
    = [] (contact::id) {};

    /**
     * Called after updating group members.
     */
    mutable std::function<void (contact::id /*group_id*/
        , std::vector<contact::id> /*added*/
        , std::vector<contact::id> /*removed*/)> group_members_updated
    = [] (contact::id, std::vector<contact::id>, std::vector<contact::id>) {};

    /**
     * Requested file/resource not found, corrupted or permission denied.
     */
    mutable std::function<void (contact::id /*requester*/
        , file::id /*file_id*/)> file_error
    = [] (contact::id, file::id) {};
};

} // namespace chat
