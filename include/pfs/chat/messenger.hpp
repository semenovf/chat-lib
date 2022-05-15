////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
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
#include "message.hpp"
#include "message_store.hpp"
#include "protocol.hpp"
#include "serializer.hpp"
#include "pfs/fmt.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <cassert>

namespace chat {

////////////////////////////////////////////////////////////////////////////////
//  ______________________________________________________________
//  |                                                            |
//  |   chat-lib                                                 |
//  |                                                            |
//  |   ------------------------      -----------------------    |
//  |   |    Contact manager   |      |Message store manager|    |
//  |   ------------------------      -----------------------    |
//  |               ^                           ^                |
//  |               |                           |                |
//  |               |                           |                |
//  |               v                           v                |
//  |       --------------------------------------------         |
//  |       |                                          |         |
//  |       |           M E S S E N G E R              |         |
//  |       |                                          |         |
//  |       --------------------------------------------         |
//  |                                           ^                |
//  |                                           |                |
//  |___________________________________________|________________|
//                                              |
//                                              |
//                                              v
//                                   -----------------------
//                                   |   Delivery manager  |
//                                   -----------------------
//                                           ^    |
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
// Frontend               Messenger                     Delivery      Messenger
//                          author                      manager       addressee
// ---------              ---------                     --------      ---------
//     |                      |                            |              |
//     |                      |      1. message ID         |              |
//     |                      |      2. author ID          |              |
//     |   dispatch_message   |      3. creation time      |              |
//     |--------------------->| (1)  4. content            |          (1')|
//     |                      |--------------------------->|------------->|
//     |                      |                            |              |
//     |                      |      1. message ID         |              |
//     |                      |      2. addressee ID       |              |
//     |                      | (2') 3. delivered time     |           (2)|
//     |                      |<---------------------------|<-------------|
//     |                      |                            |              |
//     |                      |      1. message ID         |              |
//     |                      |      2. addressee ID       |              |
//     |                      | (3') 3. read time          |           (3)|
//     |                      |<---------------------------|<-------------|
//     |                      |                            |              |
//     |                      |      1. message ID         |              |
//     |                      |      2. author ID          |              |
//     |                      |      3. modification time  |              |
//     |                      | (4)  4. content            |          (4')|
//     |                      |----------------------------|<------------>|
//     |                      |                            |              |
//     |                      |               ...          |              |
//     |                      |                            |              |
//     |                      | (4)                        |          (4')|
//     |                      |----------------------------|------------->|
//     |                      |                            |              |
//
// Step (1-1') - dispatching message
// Step (2-2') - delivery notification
// Step (3-3') - read notification
// Step (4-4') - modification notification
//
// Step (4-4') can happen zero or more times.
//
template <typename ContactManagerBackend
    , typename MessageStoreBackend
    , typename SerializerBackend>
class messenger
{
public:
    using contact_type = contact::contact;
    using person_type  = contact::person;
    using group_type   = contact::group;
//     using message_credentials_type = message::message_credentials;
    using contact_manager_type     = contact_manager<ContactManagerBackend>;
    using message_store_type       = message_store<MessageStoreBackend>;
    using serializer_type          = serializer<SerializerBackend>;
//     using icon_library_type    = typename ...;
//     using media_cache_type     = typename ...;

    using conversation_type = typename message_store_type::conversation_type;

public:
    static message::message_id const CONTACT_MESSAGE;

private:
    std::unique_ptr<contact_manager_type> _contact_manager;
    std::unique_ptr<message_store_type>   _message_store;
    contact::id_generator _contact_id_generator;
    message::id_generator _message_id_generator;

public: // Callbacks
    mutable std::function<void (std::string const &)> failure;

    mutable std::function<bool (
          contact::contact_id
        , message::message_id
        , std::string const & /*data*/)> dispatch_data;

    mutable std::function<void (contact::contact_id /*addressee*/
        , message::message_id /*message_id*/
        , pfs::utc_time_point /*dispatched_time*/)> message_dispatched;

    mutable std::function<void (contact::contact_id /*author*/
        , message::message_id /*message_id*/)> message_received;

    mutable std::function<void (contact::contact_id /*addressee*/
        , message::message_id /*message_id*/
        , pfs::utc_time_point /*delivered_time*/)> message_delivered;

    mutable std::function<void (contact::contact_id /*addressee*/
        , message::message_id /*message_id*/
        , pfs::utc_time_point /*read_time*/)> message_read;

    mutable std::function<void (contact::contact_id)> contact_added;

public:
    messenger (std::unique_ptr<contact_manager_type> && contact_manager
        , std::unique_ptr<message_store_type> && message_store)
        : _contact_manager(std::move(contact_manager))
        , _message_store(std::move(message_store))
    {
        // Set default failure callback
        failure = [] (std::string const & errstr) {
            fmt::print(stderr, "ERROR: {}\n", errstr);
        };
    }

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
     * Total contacts count.
     */
    std::size_t contacts_count () const
    {
        return _contact_manager->count();
    }

    /**
        * Get members of the specified group.
        */
    std::vector<contact::contact> members (contact::contact_id group_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(fmt::format("attempt to get members of non-existent group: {}"
                , group_id));
            return std::vector<contact::contact>{};
        }

        return group_ref.members();
    }

    /**
     * Count of contacts in specified group.
     */
    std::size_t members_count (contact::contact_id group_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(fmt::format("attempt to get members count of non-existent group: {}"
                , group_id));
            return 0;
        }

        return group_ref ? group_ref.count() : 0;
    }

    /**
     * Total count of contacts with specified @a type.
     */
    std::size_t contacts_count (contact::type_enum type) const
    {
        return _contact_manager->count(type);
    }

    /**
     * Add person contact.
     *
     * @return Identifier of just added contact or @c chat::contact::contact_id{}
     *         on error.
     */
    contact::contact_id add (contact::person c)
    {
        if (c.id == contact::contact_id{})
            c.id = _contact_id_generator.next();

        auto success = _contact_manager->add(c);
        return success ? c.id : contact::contact_id{};
    }

    /**
     * Add group contact.
     *
     * @return Identifier of just added contact or @c chat::contact::contact_id{}
     *         on error.
     */
    contact::contact_id add (contact::group g, contact::contact_id creator_id)
    {
        if (g.id == contact::contact_id{})
            g.id = _contact_id_generator.next();

        auto success = _contact_manager->add(g, creator_id);
        return success ? g.id : contact::contact_id{};
    }

    /**
     * Update contact.
     *
     * @return @c true if contact updated successfully, @c false on error or
     *         contact does not exist.
     */
    bool update (contact::contact const & c)
    {
        return _contact_manager->update(c);
    }

    bool update (contact::person const & c)
    {
        return _contact_manager->update(c);
    }

    bool update (contact::group const & g)
    {
        return _contact_manager->update(g);
    }

    /**
     * Removes contact.
     *
     * @details If @a id is a group then group contact and all memberships
     *          will be removed. If @a id is a person contact membership will
     *          be removed in case of group participation.
     */
    void remove (contact::contact_id id)
    {
        _contact_manager->remove(id);
    }

    /**
     * Get contact by @a id. On error returns invalid contact.
     */
    contact::contact
    contact (contact::contact_id id) const noexcept
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
     * @throw debby::error on storage error.
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
     * @throw debby::error on storage error.
     */
    template <typename F>
    void for_each_until (F && f)
    {
        _contact_manager->for_each_until(std::forward<F>(f));
    }

    /**
     * Adds member specified by @a member_id to the group specified by @a group_id.
     *
     * @return @c false on error or @c true if contact added was successfully
     *         or it is already a member of the specified group.
     */
    bool add_member (contact::contact_id group_id, contact::contact_id member_id)
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(fmt::format("attempt to add member into non-existent group: {}"
                , group_id));
            return false;
        }

        return group_ref.add_member(member_id);
    }

    /**
     * Removes all members from the group specified by @a group_id.
     *
     */
    void remove_all_members (contact::contact_id group_id)
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(fmt::format("attempt to remove all members from non-existent group: {}"
                , group_id));
            return;
        }

        group_ref.remove_all_members();
    }

    /**
     * Checks if contact @a member_id is the member of group @a group_id.
     */
    bool is_member_of (contact::contact_id member_id, contact::contact_id group_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref)
            return false;

        return group_ref.is_member_of(member_id);
    }

    conversation_type conversation (contact::contact_id addressee_id) const
    {
        // Check for contact exists
        auto c = contact(addressee_id);

        if (!is_valid(c)) {
            // Invalid addressee ID to generate invalid conversation
            failure(fmt::format("No contact found: {}", addressee_id));
            addressee_id = contact::contact_id{};
            return conversation_type{};
        }

        return _message_store->conversation(addressee_id);
    }

    /**
     * Total unread messages count.
     */
    std::size_t unread_messages_count () const
    {
        std::size_t result = 0;

        for_each_contact([this, & result] (contact::contact const & c) {
            auto conv = conversation(c.id);

            if (conv)
                result += conv.unread_messages_count();
        });

        return result;
    }

    template <typename F>
    bool transaction (F && op) noexcept
    {
        return _contact_manager->transaction(std::forward<F>(op));
    }

    /**
     * Dispatch message (original or edited)
     */
    bool dispatch_message (contact::contact_id addressee
        , message::message_credentials const & msg)
    {
        protocol::original_message m;
        m.message_id    = msg.id;
        m.author_id     = msg.author_id;
        m.creation_time = msg.creation_time;
        m.content       = msg.contents.has_value() ? to_string(*msg.contents) : std::string{};

        typename serializer_type::output_packet_type out {};
        out << m;
        auto success = dispatch_data(addressee, msg.id, out.data());
        return success;
    }

    /**
     * Dispatch read notification.
     */
    bool dispatch_read_notification (contact::contact_id addressee_id
        , message::message_id message_id
        , pfs::utc_time_point read_time)
    {
        // Process (mark as read) incoming message.
        process_read_notification(addressee_id, message_id, read_time);

        protocol::read_notification m;
        m.message_id = message_id;
        m.addressee_id = my_contact().id; // Addressee is me
        m.read_time = read_time;

        typename serializer_type::output_packet_type out {};
        out << m;
        auto success = dispatch_data(addressee_id, message_id, out.data());
        return success;
    }

    /**
     * Dispatch contact credentials.
     */
    bool dispatch_contact (contact::contact_id addressee)
    {
        auto me = my_contact();

        protocol::contact_credentials m {{
              me.id
            , me.id
            , me.alias
            , me.avatar
            , me.description
            , chat::contact::type_enum::person
        }};

        typename serializer_type::output_packet_type out {};
        out << m;
        auto success = dispatch_data(addressee, CONTACT_MESSAGE, out.data());
        return success;
    }

    /**
     * Dispatch messages limited by @a max_count that not dispatched or not delivered
     * (received on opponent side) status.
     */
    void dispatch_delayed_messages (contact::contact_id addressee_id, int max_count = -1)
    {
        auto conv = conversation(addressee_id);

        if (conv) {
            conv.for_each([this, addressee_id] (message::message_credentials const & m) {
                if (m.author_id != addressee_id) {
                    if (!m.dispatched_time)
                        dispatch_message(addressee_id, m);
                    else if (!m.delivered_time)
                        dispatch_message(addressee_id, m);
                }
            }, max_count);
        }
    }

    /**
     * Process received data.
     */
    void process_received_data (contact::contact_id author, std::string const & data)
    {
        typename serializer_type::input_packet_type in {data};
        protocol::packet_type_enum packet_type;
        in >> packet_type;

        switch (packet_type) {
            case protocol::packet_type_enum::contact_credentials: {
                protocol::contact_credentials m;
                in >> m;

                switch (m.contact.type) {
                    case contact::type_enum::person: {
                        contact::person p;
                        p.id = m.contact.id;
                        p.alias = std::move(m.contact.alias);
                        p.avatar = std::move(m.contact.avatar);
                        p.description = std::move(m.contact.description);

                        auto id = add(std::move(p));

                        if (id != contact::contact_id{})
                            contact_added(id);
                        break;
                    }

                    case contact::type_enum::group: {
                        contact::group g;
                        g.id = m.contact.id;
                        g.creator_id = m.contact.creator_id;
                        g.alias = std::move(m.contact.alias);
                        g.avatar = std::move(m.contact.avatar);
                        g.description = std::move(m.contact.description);

                        auto id = add(std::move(g), m.contact.creator_id);

                        if (id != contact::contact_id{})
                            contact_added(id);

                        break;
                    }

                    case contact::type_enum::channel:
                        // TODO Implement
                        break;

                    default:
                        failure(fmt::format("Unsupported contact type: {}"
                            , static_cast<int>(m.contact.type)));
                        break;
                }

                break;
            }

            case protocol::packet_type_enum::original_message: {
                protocol::original_message m;
                in >> m;
                auto conv = this->conversation(m.author_id);

                if (conv) {
                    TRY {
                        message::content content{m.content};
                        conv.save_incoming(m.message_id
                            , m.author_id
                            , m.creation_time
                            , to_string(content));

                        auto received_time = pfs::current_utc_time_point();
                        conv.mark_received(m.message_id, received_time);

                        // Send notification
                        dispatch_received_notification(m.author_id, m.message_id
                            , received_time);

                        // Notify message received
                        this->message_received(m.author_id, m.message_id);
                    } CATCH (error ex) {
#if PFS__EXCEPTIONS_ENABLED
                        failure(fmt::format("Bad/corrupted content in "
                            "incoming message {} from: {}"
                            , m.message_id
                            , author));
#endif
                    }
                }
                break;
            }

            case protocol::packet_type_enum::delivery_notification: {
                protocol::delivery_notification m;
                in >> m;
                process_delivered_notification(m.addressee_id, m.message_id, m.delivered_time);
                break;
            }

            case protocol::packet_type_enum::read_notification: {
                protocol::read_notification m;
                in >> m;

                // Process (mark as read) outgoing message.
                process_read_notification(m.addressee_id, m.message_id, m.read_time);
                break;
            }

            case protocol::packet_type_enum::edited_message: {
                protocol::edited_message m;
                in >> m;
                // TODO Implement
                break;
            }

            default:
                failure(fmt::format("Bad message received from: {}", author));
                break;
        }
    }

    /**
     * Process notification of message dispatched to @a addressee.
     */
    void dispatched (contact::contact_id addressee
        , message::message_id message_id
        , pfs::utc_time_point dispatched_time)
    {
        // This is a contact message, ignore it
        if (message_id == CONTACT_MESSAGE)
            return;

        auto conv = this->conversation(addressee);

        if (conv) {
            conv.mark_dispatched(message_id, dispatched_time);
            this->message_dispatched(addressee, message_id, dispatched_time);
        }
    }

    void wipe ()
    {
        _contact_manager->wipe();
        _message_store->wipe();
    }

private:
    /**
     * Dispatch received notification.
     */
    bool dispatch_received_notification (contact::contact_id addressee_id
        , message::message_id message_id
        , pfs::utc_time_point received_time)
    {
        protocol::delivery_notification m;
        m.message_id     = message_id;
        m.addressee_id   = my_contact().id; // Addressee is me
        m.delivered_time = received_time;

        typename serializer_type::output_packet_type out {};
        out << m;
        auto success = dispatch_data(addressee_id, message_id, out.data());
        return success;
    }

    /**
     * Process notification of message delivered to @a addressee.
     */
    void process_delivered_notification (contact::contact_id addressee
        , message::message_id message_id
        , pfs::utc_time_point delivered_time)
    {
        auto conv = this->conversation(addressee);

        if (conv) {
            conv.mark_delivered(message_id, delivered_time);
            this->message_delivered(addressee, message_id, delivered_time);
        }
    }

    /**
     * Process notification of message read by @a addressee.
     */
    void process_read_notification (contact::contact_id addressee
        , message::message_id message_id
        , pfs::utc_time_point read_time)
    {
        auto conv = this->conversation(addressee);

        if (conv) {
            conv.mark_read(message_id, read_time);
            this->message_read(addressee, message_id, read_time);
        }
    }
};

template <typename ContactManagerBackend
    , typename MessageStoreBackend
    , typename SerializerBackend>
message::message_id const
messenger<ContactManagerBackend, MessageStoreBackend, SerializerBackend>
    ::CONTACT_MESSAGE {"00000000000000000000000001"_uuid};

} // namespace chat
