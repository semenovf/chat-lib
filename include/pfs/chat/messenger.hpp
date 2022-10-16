////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.17 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "contact_manager.hpp"
#include "error.hpp"
#include "file.hpp"
#include "file_cache.hpp"
#include "message.hpp"
#include "message_store.hpp"
#include "protocol.hpp"
#include "serializer.hpp"
#include "pfs/assert.hpp"
#include "pfs/fmt.hpp"
#include "pfs/i18n.hpp"
#include "pfs/log.hpp"
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

namespace chat {

////////////////////////////////////////////////////////////////////////////////
//  ______________________________________________________________
//  |                                                            |
//  |   chat-lib                                                 |
//  |                                                            |
//  |   -----------------  -----------   ----------------------- |
//  |   |Contact manager|  |File cache|  |Message store manager| |
//  |   -----------------  ------------  ----------------------- |
//  |               ^           ^                 ^              |
//  |               |           |                 |              |
//  |               v           v                 v              |
//  |       --------------------------------------------         |
//  |       |                                          |         |
//  |       |           M E S S E N G E R              |         |
//  |       |                                          |         |
//  |       --------------------------------------------         |
//  |                                           ^                |
//  |                                           |                |
//  |                                           v                |
//  |                                -----------------------     |
//  |                                |   Delivery manager  |     |
//  |                                -----------------------     |
//  |                                        ^    |              |
//  |________________________________________|____|______________|
//                                           |    |
//                                           |    v
//                                      Communication media
//                                     (IPC, LAN, Internet, ...)
//
// Communication media refers to the ways, means or channels of transmitting
// message from sender to the receiver.
//
////////////////////////////////////////////////////////////////////////////////
//                          Message lifecycle
//
// Frontend          Messenger                     Delivery      Messenger
//                   author                        manager       addressee
// ---------         ---------                     --------      ---------
//     |                  |                            |              |
//     |                  |      1. message ID         |              |
//     |                  |      2. author ID          |              |
//     |                  |      3. conversation ID    |              |
//     | dispatch_message |      4. creation time      |              |
//     |----------------->| (1)  5. content            |              |
//     |                  |--------------------------->|          (1')|
//     |                  |                            |------------->|
//     |                  |      1. message ID         |              |
//     |                  |      2. addressee ID       |              |
//     |                  |      3. conversation ID'   |           (2)|
//     |                  | (2') 4. delivered time     |<-------------|
//     |                  |<---------------------------|              |
//     |                  |                            |              |
//     |                  |      1. message ID         |              |
//     |                  |      2. addressee ID       |              |
//     |                  |      3. conversation ID'   |           (3)|     (3")
//     |                  | (3') 4. read time          |<-------------|-------->
//     |                  |<---------------------------|              |
//     |                  |                            |              |
//     |                  |      1. message ID         |              |
//     |                  |      2. author ID          |              |
//     |                  |      3. conversation ID    |              |
//     |                  |      4. modification time  |              |
//     |                  | (4)  5. content            |              |
//     |                  |--------------------------->|          (4')|
//     |                  |                            |------------->|
//     |                  |               ...          |              |
//     |                  | (4)                        |              |
//     |                  |--------------------------->|          (4')|
//     |                  |                            |------------->|
//     |                  |                            |              |
//
// Step (1-1') - dispatching message
// Step (2-2') - delivery notification
// Step (3-3') - read notification
// Step (3")   - mark as read on receiver side
// Step (4-4') - modification notification
//
// Step (4-4') can happen zero or more times.
//
// conversation ID  -> Author or group ID
// conversation ID' -> Addressee or group ID
////////////////////////////////////////////////////////////////////////////////
/**
 * Terms
 *
 * -# @c Addressee Side (person) of the conversation that received the message.
 *      Synonym for @c Receiver.
 * -# @c Addresser Side (person) of the conversation that send the message.
 *      Synonym for @c Sender.
 * -# @c Author Side (person) of the conversation that create and send the message.
 * -# @c Conversation Conversation (dialogue) as is. Can be personal conversation,
 *         group conversation or channel. Conversation associates with two
 *         (personal) or more (group conversation or channel) sides.
 * -# @c Conversation @c credentials Data that dsscribes conversation.
 * -# @c Contact Is a synonym for conversation credentials.
 * -# @c Message The unit of the conversation. Has author (creator) and destination
 *          (another side of the conversation).
 * -# @c Opponent Opposite side of the conversation. Can be synonym for
 *          @c Addressee / @c Receiver.
 * -# @c Receiver Side (person) of the conversation that received the message.
 *          Synonym for @c Addressee.
 * -# @c Sender Side (person) of the conversation that send the message.
 */
////////////////////////////////////////////////////////////////////////////////
#ifndef CHAT__MESSENGER_TAG
#   define CHAT__MESSENGER_TAG "chat::messenger"
#endif

template <typename ContactManagerBackend
    , typename MessageStoreBackend
    , typename FileCacheBackend
    , typename SerializerBackend>
class messenger
{
public:
    using contact_type = contact::contact;
    using person_type  = contact::person;
    using group_type   = contact::group;
    using contact_manager_type  = contact_manager<ContactManagerBackend>;
    using message_store_type    = message_store<MessageStoreBackend>;
    using file_cache_type       = file_cache<FileCacheBackend>;
    using serializer_type       = serializer<SerializerBackend>;
//     using icon_library_type    = typename ...;

    using conversation_type = typename message_store_type::conversation_type;

private:
    std::unique_ptr<contact_manager_type>  _contact_manager;
    std::unique_ptr<message_store_type>    _message_store;
    std::unique_ptr<file_cache_type>       _file_cache;
    contact::id_generator _contact_id_generator;
    message::id_generator _message_id_generator;
    file::id_generator    _file_id_generator;

public: // Callbacks
    /**
     * Called to dispatch data (pass to delivery manager).
     *
     * @param message_address Message address.
     */
    mutable std::function<void (contact::id /*addressee_id*/
        , std::string const & /*data*/)> dispatch_data
    = [] (contact::id, std::string const &) {};

    /**
     * Called when file/attachment request received.
     */
    mutable std::function<void (contact::id /*addressee_id*/
        , file::id /*file_id*/
        , pfs::filesystem::path const & /*path*/)> dispatch_file
    = [] (contact::id, file::id, pfs::filesystem::path const &) {};

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

public:
    messenger (std::unique_ptr<contact_manager_type> contact_manager
        , std::unique_ptr<message_store_type> message_store
        , std::unique_ptr<file_cache_type> file_cache)
        : _contact_manager(std::move(contact_manager))
        , _message_store(std::move(message_store))
        , _file_cache(std::move(file_cache))
    {}

    ~messenger () = default;

    messenger (messenger const &) = delete;
    messenger & operator = (messenger const &) = delete;

    messenger (messenger &&) = delete;
    messenger & operator = (messenger &&) = delete;

    operator bool () const noexcept
    {
        return *_contact_manager
            && *_message_store;
    }

    /**
     * Local contact.
     */
    contact::person my_contact () const
    {
        return _contact_manager->my_contact();
    }

    /**
     * Changes @a alias for my contact.
     */
    void change_my_alias (std::string const & alias)
    {
        _contact_manager->change_my_alias(alias);
    }

    /**
     * Changes @a avatar for my contact.
     */
    void change_my_avatar (std::string const & avatar)
    {
        _contact_manager->change_my_avatar(avatar);
    }

    /**
     * Changes description for my contact.
     */
    void change_my_desc (std::string const & desc)
    {
        _contact_manager->change_my_description(desc);
    }

    /**
     * Total contacts count.
     */
    std::size_t contacts_count () const
    {
        return _contact_manager->count();
    }

////////////////////////////////////////////////////////////////////////////////
// Group contact specific methods
////////////////////////////////////////////////////////////////////////////////
    /**
     * Adds member specified by @a member_id to the group specified by @a group_id.
     */
    void add_member (contact::id group_id, contact::id member_id
        , std::error_code & ec) noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            ec = make_error_code(errc::group_not_found);
            return;
        }

        group_ref.add_member(member_id);

        LOG_TRACE_3(CHAT__MESSENGER_TAG ": add_member: group_id={}; member_id={}"
            , group_id, member_id);
    }

    /**
     * Adds member specified by @a member_id to the group specified by @a group_id.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    inline void add_member (contact::id group_id, contact::id member_id)
    {
        std::error_code ec;
        add_member(group_id, member_id, ec);

        if (ec)
            throw error{ec, to_string(group_id)};
    }

    /**
     * Removes member specified by @a member_id from the group specified
     * by @a group_id.
     */
    void remove_member (contact::id group_id, contact::id member_id
        , std::error_code & ec) noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            ec = make_error_code(errc::group_not_found);
            return;
        }

        group_ref.remove_member(member_id);
    }

    /**
     * Removes member specified by @a member_id from the group specified
     * by @a group_id.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    inline void remove_member (contact::id group_id, contact::id member_id)
    {
        std::error_code ec;
        remove_member(group_id, member_id, ec);

        if (ec)
            throw error{ec, to_string(group_id)};
    }

    /**
     * Removes all members from the group specified by @a group_id.
     */
    void remove_all_members (contact::id group_id, std::error_code & ec) noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            ec = make_error_code(errc::group_not_found);
            return;
        }

        group_ref.remove_all_members();
    }

    /**
     * Removes all members from the group specified by @a group_id.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    inline void remove_all_members (contact::id group_id)
    {
        std::error_code ec;
        remove_all_members(group_id, ec);

        if (ec)
            throw error{ec, to_string(group_id)};
    }

    /**
     * Gets members of the specified group. Returns empty vector on error.
     */
    std::vector<contact::contact> members (contact::id group_id
        , std::error_code & ec) const noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            ec = make_error_code(errc::group_not_found);
            return std::vector<contact::contact>{};
        }

        return group_ref.members();
    }

    /**
     * Gets members of the specified group.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    std::vector<contact::contact> members (contact::id group_id) const
    {
        std::error_code ec;
        auto result = members(group_id, ec);

        if (ec)
            throw error{ec, to_string(group_id)};

        return result;
    }

    /**
     * Get member identifiers of the specified group.
     */
    std::vector<contact::id> member_ids (contact::id group_id
        , std::error_code & ec) const noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            ec = make_error_code(errc::group_not_found);
            return std::vector<contact::id>{};
        }

        return group_ref.member_ids();
    }

    /**
     * Get member identifiers of the specified group.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    inline std::vector<contact::id> member_ids (contact::id group_id) const
    {
        std::error_code ec;
        auto result = member_ids(group_id, ec);

        if (ec)
            throw error{ec, to_string(group_id)};

        return result;
    }

    /**
     * Count of contacts in specified group. Return @c 0 on error.
     */
    std::size_t members_count (contact::id group_id, std::error_code & ec) const noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        // Attempt to get members count of non-existent group
        if (!group_ref) {
            ec = make_error_code(errc::group_not_found);
            return 0;
        }

        return group_ref.count();
    }

    /**
     * Count of contacts in specified group.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    inline std::size_t members_count (contact::id group_id) const
    {
        std::error_code ec;
        auto result = members_count(group_id, ec);

        // Attempt to get members count of non-existent group
        if (ec)
            throw error{ec, to_string(group_id)};

        return result;
    }

    /**
     * Checks if contact @a member_id is the member of group @a group_id.
     * Returns @c false on error.
     */
    bool is_member_of (contact::id group_id, contact::id member_id
        , std::error_code & ec) const noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            ec = make_error_code(errc::group_not_found);
            return false;
        }

        return group_ref.is_member_of(member_id);
    }

    /**
     * Checks if contact @a member_id is the member of group @a group_id.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    inline bool is_member_of (contact::id group_id, contact::id member_id) const
    {
        std::error_code ec;
        auto result = is_member_of(group_id, member_id, ec);

        if (ec)
            throw error{ec, to_string(group_id)};

        return result;
    }

    /**
     * Total count of contacts with specified @a type.
     */
    std::size_t contacts_count (conversation_enum type) const
    {
        return _contact_manager->count(type);
    }

    /**
     * Add personal or group contact.
     *
     * @return Identifier of just added contact or @c chat::contact::id{}
     *         if contact already exists with specified identifier.
     */
    template <typename ConcreteContactType>
    typename std::enable_if<std::is_same<ConcreteContactType, contact::person>::value
        || std::is_same<ConcreteContactType, contact::group>::value, contact::id>::type
    add (ConcreteContactType c)
    {
        if (c.contact_id == contact::id{})
            c.contact_id = _contact_id_generator.next();

        if (_contact_manager->add(c)) {
            contact_added(c.contact_id);
            return c.contact_id;
        }

        return contact::id{};
    }

    /**
     * Update contact (personal or group).
     *
     * @return @c true if contact updated successfully, @c false on error or
     *         contact does not exist.
     */
    template <typename ConcreteContactType>
    typename std::enable_if<std::is_same<ConcreteContactType, contact::person>::value
        || std::is_same<ConcreteContactType, contact::group>::value, bool>::type
    update (ConcreteContactType const & c)
    {
        if (!_contact_manager->update(c))
            return false;

        contact_updated(c.contact_id);
        return true;
    }

    /**
     * Updates or adds (if update is not possible) person or group contact.
     *
     * @brief @c ConcreteContactType must be @c contact::person or
     *        @c contact::group.
     *
     * @return Identifier of updated or added contact.
     *
     * @throw chat::error (@c errc::storage_error) on storage error.
     */
    template <typename ConcreteContactType>
    typename std::enable_if<std::is_same<ConcreteContactType, contact::person>::value
        || std::is_same<ConcreteContactType, contact::group>::value, contact::id>::type
    update_or_add (ConcreteContactType p)
    {
        if (p.contact_id == contact::id{}) {
            p.contact_id = _contact_id_generator.next();
            return add(p);
        }

        if (!update(p)) {
            auto id = add(p);
            PFS__ASSERT(id != contact::id{}, "");
            return id;
        }

        return p.contact_id;
    }

    /**
     * Removes contact.
     *
     * @details If @a id is a group then group contact and all memberships
     *          will be removed. If @a id is a person contact membership will
     *          be removed in case of group participation.
     */
    void remove (contact::id id)
    {
        _contact_manager->remove(id);
        wipe_conversation(id);
        contact_removed(id);
    }

    /**
     * Get contact by @a id. On error returns invalid contact.
     */
    contact::contact
    contact (contact::id id) const noexcept
    {
        return _contact_manager->get(id);
    }

    /**
     * Get contact by @a offset. On error returns invalid contact.
     */
    contact::contact
    contact (int offset) const noexcept
    {
        return _contact_manager->get(offset);
    }

    /**
     * Fetch all contacts and process them by @a f
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    template <typename F>
    void for_each_contact (F && f) const
    {
        _contact_manager->for_each(std::forward<F>(f));
    }

    /**
     * Fetch all contacts and process them by @a f until @f does not
     * return @c false.
     *
     * @throw chat::error{errc::storage_error} on storage error.
     */
    template <typename F>
    void for_each_until (F && f)
    {
        _contact_manager->for_each_until(std::forward<F>(f));
    }

    conversation_type conversation (contact::id conversation_id) const
    {
        // Check for contact exists
        auto c = contact(conversation_id);

        if (!is_valid(c))
            return conversation_type{};

        auto convers = _message_store->conversation(conversation_id);

        convers.cache_outcome_file = [this] (pfs::filesystem::path const & path) {
            return _file_cache->store_outgoing_file(path);
        };

        return convers;
    }

    /**
     * Clears conversation messages and attachments
     */
    void clear_conversation (contact::id conversation_id)
    {
        auto convers = _message_store->conversation(conversation_id);

        if (convers) {
            convers.clear();
            // TODO Implement clear file cache with removing incoming files.
            //_file_cache->clear(conversation_id);
        }
    }

    void wipe_conversation (contact::id conversation_id)
    {
        auto convers = _message_store->conversation(conversation_id);

        if (convers) {
            convers.wipe();
            // TODO Implement removing file cache with removing download directory.
            //_file_cache->wipe(conversation_id);
        }
    }

    /**
     * Total unread messages count.
     */
    std::size_t unread_messages_count () const
    {
        std::size_t result = 0;

        for_each_contact([this, & result] (contact::contact const & c) {
            auto conv = conversation(c.contact_id);

            if (conv)
                result += conv.unread_messages_count();
        });

        return result;
    }

    // FIXME DEPRECATED
    template <typename F>
    bool transaction (F && op) noexcept
    {
        return _contact_manager->transaction(std::forward<F>(op));
    }

    /**
     * Dispatch message (original or edited) for person or group.
     *
     * @param conv Conversation that the message belongs to.
     * @param message_id Message identifier.
     *
     * @throw error{errc::bad_conversation_type} Bad conversation type.
     */
    void dispatch_message (conversation_type const & conv, message::id message_id)
    {
        if (!conv)
            return;

        auto addressee = contact(conv.id());
        auto msg = conv.message(message_id);

        protocol::regular_message m;
        m.message_id = msg->message_id;
        m.author_id  = msg->author_id;
        m.conversation_id = is_person(addressee) ? msg->author_id : conv.id();
        m.mod_time   = msg->modification_time;
        m.content    = msg->contents.has_value()
            ? to_string(*msg->contents)
            : std::string{};

        typename serializer_type::output_packet_type out {};
        out << m;
        dispatch_multicast(addressee, out.data());
    }

    /**
     * This is a convenient method for dispatch message
     * (@see messenger::dispatch_message(conversation_type const &, message::id)).
     */
    inline void dispatch_message (contact::id conversation_id, message::id message_id)
    {
        dispatch_message(this->conversation(conversation_id), message_id);
    }

    /**
     * Mark received message as read and dispatch read notification to message
     * author or members of conversation group.
     *
     * @param conversation_id Conversation identifier.
     * @param message_id Message identifier.
     * @param read_time Read time in UTC.
     *
     * @throw chat::error{errc::conversation_not_found} if conversation not found
     *        specified by @a message_address.
     * @throw chat::error{errc::message_not_found} if message not found
     *        specified by @a message_address.
     */
    void dispatch_read_notification (contact::id conversation_id
        , message::id message_id
        , pfs::utc_time_point read_time)
    {
        //
        // Mark incoming message as read
        //
        auto conv = this->conversation(conversation_id);

        if (!conv) {
            throw chat::error{errc::conversation_not_found
                , to_string(conversation_id)};
        }

        process_read_notification(conv, message_id, read_time);

        //
        // Dispatch message read notification to message sender.
        //
        auto conv_contact = contact(conv.id());

        protocol::read_notification m;
        m.message_id = message_id;
        m.conversation_id = is_person(conv_contact)
            ? my_contact().contact_id : conversation_id;
        m.read_time = read_time;

        typename serializer_type::output_packet_type out {};
        out << m;
        dispatch_data(conv_contact.contact_id, out.data());
    }

    /**
     * Dispatch contact credentials.
     */
    void dispatch_contact (contact::id addressee_id) const
    {
        auto me = my_contact();

        protocol::contact_credentials c {{
              me.contact_id
            , me.alias
            , me.avatar
            , me.description
            , me.contact_id
            , conversation_enum::person
        }};

        typename serializer_type::output_packet_type out {};
        out << c;
        dispatch_data(addressee_id, out.data());
    }

    /**
     * Dispatch group contact and list of group members.
     * Used when group created or updated localy.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found
     *        by @a group_id.
     */
    void dispatch_group (contact::id addressee_id, contact::id group_id) const
    {
        // Skip own contact
        if (addressee_id == my_contact().contact_id)
            return;

        auto g = _contact_manager->get(group_id);

        if (!is_valid(g))
            throw error{errc::group_not_found, to_string(group_id)};

        // Send group contact
        protocol::contact_credentials c {{
              g.contact_id
            , g.alias
            , g.avatar
            , g.description
            , g.creator_id
            , conversation_enum::group
        }};

        {
            typename serializer_type::output_packet_type out {};
            out << c;
            dispatch_data(addressee_id, out.data());
        }

        // Send group members
        auto gref = _contact_manager->gref(group_id);
        auto members = gref.members();

        protocol::group_members gm;
        gm.group_id = group_id;

        for (auto const & c: members)
            gm.members.push_back(c.contact_id);

        {
            typename serializer_type::output_packet_type out {};
            out << gm;
            dispatch_data(addressee_id, out.data());
        }
    }

    /**
     * Dispatch group removed message.
     * Used when group removed localy.
     */
    void dispatch_group_removed (contact::id addressee_id, contact::id group_id) const
    {
        // Skip own contact
        if (addressee_id == my_contact().contact_id)
            return;

        protocol::group_members gm;
        gm.group_id = group_id;

        typename serializer_type::output_packet_type out {};
        out << gm;

        dispatch_data(addressee_id, out.data());
    }

    /**
     * Dispatch self created group contacts and list of members.
     */
    void dispatch_self_created_groups (contact::id addressee_id) const
    {
        auto my_contact_id = my_contact().contact_id;

        for_each_contact([this, addressee_id, my_contact_id] (contact::contact const & c) {
            auto self_created_group = (c.type == conversation_enum::group)
                && (c.creator_id == my_contact_id);

            if (self_created_group)
                dispatch_group(addressee_id, c.contact_id);
        });
    }

    /**
     * Dispatch file request. Must be called by messenger implementer to
     * initiate attachment/file downloading.
     */
    void dispatch_file_request (contact::id addressee_id, file::id file_id)
    {
        // Skip own contact
        if (addressee_id == my_contact().contact_id)
            return;

        typename serializer_type::output_packet_type out {};
        out << protocol::file_request{file_id};

        dispatch_data(addressee_id, out.data());
    }

    void dispatch_file_error (contact::id addressee_id, file::id file_id)
    {
        // Skip own contact
        if (addressee_id == my_contact().contact_id)
            return;

        protocol::file_error m;
        m.file_id = file_id;

        typename serializer_type::output_packet_type out {};
        out << m;
        dispatch_data(addressee_id, out.data());
    }

    /**
     * Process received data. Must be called by messenger implementer to
     * process incomming data.
     *
     * @param addresser_id Data sender.
     * @param data Data received.
     *
     * @throw chat::error{errc::bad_conversation_type} Unsupported conversation type.
     * @throw chat::error{errc::group_not_found} Received conversation group
     *        specific data but conversation group not found.
     * @throw chat::error{} Bad/corrupted message content.
     * @throw chat::error{errc::bad_packet_type} Bad packet type received.
     */
    void process_incoming_data (contact::id addresser_id, std::string const & data)
    {
        typename serializer_type::input_packet_type in {data};
        protocol::packet_enum packet_type;
        in >> packet_type;

        switch (packet_type) {
            case protocol::packet_enum::contact_credentials: {
                protocol::contact_credentials cc;
                in >> cc;

                switch (cc.contact.type) {
                    case conversation_enum::person: {
                        contact::person p;
                        p.contact_id = cc.contact.contact_id;
                        p.alias = std::move(cc.contact.alias);
                        p.avatar = std::move(cc.contact.avatar);
                        p.description = std::move(cc.contact.description);

                        /*auto id = */update_or_add(std::move(p));
                        break;
                    }

                    case conversation_enum::group: {
                        contact::group g;
                        g.contact_id = cc.contact.contact_id;
                        g.creator_id = cc.contact.creator_id;
                        g.alias = std::move(cc.contact.alias);
                        g.avatar = std::move(cc.contact.avatar);
                        g.description = std::move(cc.contact.description);

                        /*auto id = */update_or_add(std::move(g));
                        break;
                    }

                    case conversation_enum::channel:
                        // TODO Implement
                        break;

                    default:
                        throw error{errc::bad_conversation_type};
                        break;
                }

                break;
            }

            case protocol::packet_enum::group_members: {
                protocol::group_members gm;
                in >> gm;

                if (gm.members.empty()) {
                    // Group removed or contact has been removed from group.
                    // So remove group locally.
                    remove(gm.group_id);
                } else {
                    auto gref = _contact_manager->gref(gm.group_id);

                    if (!gref) {
                        throw error{errc::group_not_found, to_string(gm.group_id)};
                        break;
                    }

                    auto diffs = gref.update(gm.members);

                    group_members_updated(gm.group_id
                        , std::move(diffs.added)
                        , std::move(diffs.removed));
                }

                break;
            }

            case protocol::packet_enum::regular_message: {
                protocol::regular_message m;
                in >> m;
                process_regular_message(m);
                break;
            }

            case protocol::packet_enum::delivery_notification: {
                protocol::delivery_notification m;
                in >> m;
                process_delivered_notification(m);
                break;
            }

            case protocol::packet_enum::read_notification: {
                protocol::read_notification m;
                in >> m;
                process_read_notification(m);
                break;
            }

            case protocol::packet_enum::file_request: {
                protocol::file_request m;
                in >> m;
                process_file_request(addresser_id, m);
                break;
            }

            case protocol::packet_enum::file_error: {
                protocol::file_error m;
                in >> m;
                process_file_error(addresser_id, m);
                break;
            }

            default:
                // Bad message received
                throw error{errc::bad_packet_type};
                break;
        }
    }

    /**
     * Cache incoming attachment/file in file cache.
     *
     * @details Must be called by messenger implementer when attachment/file
     *          received completely.
     */
    void commit_incoming_file (contact::id author_id, file::id file_id
        , pfs::filesystem::path const & path)
    {
        _file_cache->store_incoming_file(author_id, file_id, path);
    }

    pfs::filesystem::path incoming_file (file::id file_id) const
    {
        auto fc = _file_cache->incoming_file(file_id);
        return !!fc ? fc->path : pfs::filesystem::path{};
    }

    pfs::filesystem::path outgoing_file (file::id file_id) const
    {
        auto fc = _file_cache->outgoing_file(file_id);
        return !!fc ? fc->path : pfs::filesystem::path{};
    }

    void wipe ()
    {
        _contact_manager->wipe();
        _message_store->wipe();
    }

private:
    // Can be considered that `dispatch_data` is analog to `dispatch_unicast`.
    void dispatch_multicast (contact::contact const & addressee, std::string const & data)
    {
        switch (addressee.type) {
            case conversation_enum::person:
                dispatch_data(addressee.contact_id, data);
                break;

            case conversation_enum::group: {
                auto group_ref = _contact_manager->gref(addressee.contact_id);
                auto member_ids = group_ref.member_ids();

                for (auto const & member_id: member_ids) {
                    if (member_id != my_contact().contact_id)
                        dispatch_data(member_id, data);
                }

                break;
            }

            case conversation_enum::channel:
                // TODO Unsupported yet
                break;

            default:
                throw error{errc::bad_conversation_type};
                break;
        }
    }

    /**
     * @throw chat::error{errc::conversation_not_found} if specified in message
     *        @a m conversation not found.
     */
    void process_regular_message (protocol::regular_message const & m)
    {
        auto conv = this->conversation(m.conversation_id);

        if (!conv) {
            throw error {errc::conversation_not_found
                , to_string(m.conversation_id)};
        }


        // Can throw when bad/corrupted content in incoming message
        message::content content{m.content};

        conv.save_incoming(m.message_id
            , m.author_id
            , m.mod_time
            , to_string(content));

        auto received_time = pfs::current_utc_time_point();
        conv.mark_received(m.message_id, received_time);

        // Send notification
        dispatch_delivery_notification(m.author_id, conv.id()
            , m.message_id, received_time);

        // Notify message received
        message_received(m.author_id, m.conversation_id, m.message_id);
    }

    /**
     * Dispatch delivery (received by addressee) notification (to author only).
     */
    void dispatch_delivery_notification (contact::id author_id
        , contact::id conversation_id
        , message::id message_id
        , pfs::utc_time_point received_time)
    {
        auto conv_contact = contact(conversation_id);

        if (!is_valid(conv_contact))
            throw error{errc::contact_not_found, to_string(conversation_id)};

        auto addressee = contact(author_id);

        if (!is_valid(addressee))
            throw error{errc::contact_not_found, to_string(addressee.contact_id)};

        protocol::delivery_notification m;
        m.message_id      = message_id;
        m.conversation_id = is_person(conv_contact)
            ? my_contact().contact_id : conversation_id;
        m.delivered_time  = received_time;

        typename serializer_type::output_packet_type out {};
        out << m;
        dispatch_multicast(addressee, out.data());
    }

    /**
     * Process notification of message delivered to addressee (receiver).
     *
     * @throw chat::error{errc::conversation_not_found} if specified in
     *        notification @a m conversation not found.
     */
    void process_delivered_notification (protocol::delivery_notification const & m)
    {
        auto conv = this->conversation(m.conversation_id);

        if (!conv) {
            throw error {errc::conversation_not_found
                , to_string(m.conversation_id)};
        }

        conv.mark_delivered(m.message_id, m.delivered_time);
        message_delivered(m.conversation_id, m.message_id, m.delivered_time);
    }

    inline void process_read_notification(conversation_type & conv
        , message::id message_id, pfs::utc_time_point read_time)
    {
        conv.mark_read(message_id, read_time);
        message_read(conv.id(), message_id, read_time);
    }

    /**
     * Process notification of message read by addressee (receiver).
     *
     * @throw chat::error{errc::conversation_not_found} if specified in
     *        notification @a m conversation not found.
     */
    void process_read_notification (protocol::read_notification const & m)
    {
        auto conv = this->conversation(m.conversation_id);

        if (!conv) {
            throw error {errc::conversation_not_found
                , to_string(m.conversation_id)};
        }

        process_read_notification(conv, m.message_id, m.read_time);
    }

    /**
     * Process file request.
     */
    void process_file_request (contact::id addresser_id
        , protocol::file_request const & m)
    {
        auto fc = _file_cache->outgoing_file(m.file_id);

        if (fc) {
            dispatch_file(addresser_id, fc->file_id, fc->path);
        } else {
            // File not found in cache by specified ID.
            dispatch_file_error(addresser_id, m.file_id);
        }
    }

    void process_file_error (contact::id addresser_id
        , protocol::file_error const & m)
    {
        file_error(addresser_id, m.file_id);
    }
};

} // namespace chat
