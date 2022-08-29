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
#include "file_cache.hpp"
#include "message.hpp"
#include "message_store.hpp"
#include "protocol.hpp"
#include "serializer.hpp"
#include "pfs/assert.hpp"
#include "pfs/fmt.hpp"
#include "pfs/i18n.hpp"
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
//  |               |           |                 |              |
//  |               v           v                 v              |
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
    , typename FileCacheBackend
    , typename SerializerBackend>
class messenger
{
public:
    using contact_type = contact::contact;
    using person_type  = contact::person;
    using group_type   = contact::group;
    using contact_manager_type = contact_manager<ContactManagerBackend>;
    using message_store_type   = message_store<MessageStoreBackend>;
    using file_cache_type      = file_cache<FileCacheBackend>;
    using serializer_type      = serializer<SerializerBackend>;
//     using icon_library_type    = typename ...;

    using conversation_type = typename message_store_type::conversation_type;

public:
    static message::id const CONTACT_MESSAGE;
    static message::id const MEMBERS_MESSAGE;

private:
    std::unique_ptr<contact_manager_type> _contact_manager;
    std::unique_ptr<message_store_type>   _message_store;
    std::shared_ptr<file_cache_type>      _file_cache;
    contact::id_generator _contact_id_generator;
    message::id_generator _message_id_generator;
    file::id_generator    _file_id_generator;

public: // Callbacks
    mutable std::function<void (std::string const &)> failure;

    mutable std::function<bool (
          contact::id
        , message::id
        , std::string const & /*data*/)> dispatch_data;

    mutable std::function<void (contact::id /*addressee*/
        , message::id /*message_id*/)> about_to_dispatch_message;

    mutable std::function<void (contact::id /*addressee*/
        , message::id /*message_id*/
        , pfs::utc_time_point /*dispatched_time*/)> message_dispatched;

    mutable std::function<void (contact::id /*author*/
        , message::id /*message_id*/)> message_received;

    mutable std::function<void (contact::id /*addressee*/
        , message::id /*message_id*/
        , pfs::utc_time_point /*delivered_time*/)> message_delivered;

    mutable std::function<void (contact::id /*addressee*/
        , message::id /*message_id*/
        , pfs::utc_time_point /*read_time*/)> message_read;

    /**
     * Called after adding contact.
     */
    mutable std::function<void (contact::id)> contact_added;

    /**
     * Called after updating contact.
     */
    mutable std::function<void (contact::id)> contact_updated;

    /**
     * Called after contact removed.
     */
    mutable std::function<void (contact::id)> contact_removed;

    /**
     * Called after updating group members.
     */
    mutable std::function<void (contact::id /*group_id*/
        , std::vector<contact::id> /*added*/
        , std::vector<contact::id> /*removed*/)> group_members_updated;

public:
    messenger (std::unique_ptr<contact_manager_type> && contact_manager
        , std::unique_ptr<message_store_type> && message_store
        , std::unique_ptr<file_cache_type> && file_cache)
        : _contact_manager(std::move(contact_manager))
        , _message_store(std::move(message_store))
        , _file_cache(std::move(file_cache))
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
     *
     * @return @c false on error or @c true if contact added was successfully
     *         or it is already a member of the specified group.
     */
    bool add_member (contact::id group_id, contact::id member_id)
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(tr::f_("attempt to add member into non-existent group: {}"
                , group_id));
            return false;
        }

        return group_ref.add_member(member_id);
    }

    /**
     * Removes member specified by @a member_id from the group specified
     * by @a group_id.
     */
    void remove_member (contact::id group_id, contact::id member_id)
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(tr::f_("attempt to remove member from non-existent group: {}"
                , group_id));
            return;
        }

        group_ref.remove_member(member_id);
    }

    /**
     * Removes all members from the group specified by @a group_id.
     */
    void remove_all_members (contact::id group_id)
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(tr::f_("attempt to remove all members from non-existent group: {}"
                , group_id));
            return;
        }

        group_ref.remove_all_members();
    }

    /**
      * Get members of the specified group.
      */
    std::vector<contact::contact> members (contact::id group_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(tr::f_("attempt to get members of non-existent group: {}"
                , group_id));
            return std::vector<contact::contact>{};
        }

        return group_ref.members();
    }

    /**
      * Get member identifiers of the specified group.
      */
    std::vector<contact::id> member_ids (contact::id group_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(tr::f_("attempt to get members of non-existent group: {}"
                , group_id));
            return std::vector<contact::id>{};
        }

        return group_ref.member_ids();
    }
    /**
     * Count of contacts in specified group.
     */
    std::size_t members_count (contact::id group_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        // Attempt to get members count of non-existent group
        if (!group_ref)
            return 0;

        return group_ref ? group_ref.count() : 0;
    }

    /**
     * Checks if contact @a member_id is the member of group @a group_id.
     */
    bool is_member_of (contact::id group_id, contact::id member_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref)
            return false;

        return group_ref.is_member_of(member_id);
    }

    /**
     * Total count of contacts with specified @a type.
     */
    std::size_t contacts_count (contact::type_enum type) const
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
            if (contact_added)
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

        if (contact_updated)
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

        if (contact_removed)
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

    conversation_type conversation (contact::id addressee_id) const
    {
        // Check for contact exists
        auto c = contact(addressee_id);

        if (!is_valid(c)) {
            // Invalid addressee ID to generate invalid conversation
            //failure(fmt::format("No contact found: {}", addressee_id));
            addressee_id = contact::id{};
            return conversation_type{};
        }

        return _message_store->conversation(addressee_id);
    }

    /**
     * Load file from cache.
     *
     * @details If the file is not found in the cache, it will be stored.
     */
    file::file_credentials ensure_file (pfs::filesystem::path const & path
        , std::string const & sha256)
    {
        return _file_cache->ensure(path, sha256);
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

    template <typename F>
    bool transaction (F && op) noexcept
    {
        return _contact_manager->transaction(std::forward<F>(op));
    }

    /**
     * Dispatch message (original or edited)
     */
    bool dispatch_message (contact::id addressee
        , message::message_credentials const & msg)
    {
        protocol::original_message m;
        m.message_id    = msg.message_id;
        m.author_id     = msg.author_id;
        m.creation_time = msg.creation_time;
        m.content       = msg.contents.has_value() ? to_string(*msg.contents) : std::string{};

        typename serializer_type::output_packet_type out {};
        out << m;

        if (about_to_dispatch_message)
            about_to_dispatch_message(addressee, msg.message_id);

        PFS__ASSERT(!!dispatch_data, "dispatch_data callback must be initialized");
        auto success = dispatch_data(addressee, msg.message_id, out.data());
        return success;
    }

    /**
     * Dispatch read notification.
     */
    void dispatch_read_notification (contact::id addressee_id
        , message::id message_id
        , pfs::utc_time_point read_time)
    {
        // Process (mark as read) incoming message.
        process_read_notification(addressee_id, message_id, read_time);

        protocol::read_notification m;
        m.message_id = message_id;
        m.addressee_id = my_contact().contact_id; // Addressee is me
        m.read_time = read_time;

        typename serializer_type::output_packet_type out {};
        out << m;
        dispatch_data(addressee_id, message_id, out.data());
    }

    /**
     * Dispatch contact credentials.
     */
    void dispatch_contact (contact::id addressee) const
    {
        auto me = my_contact();

        protocol::contact_credentials c {{
              me.contact_id
            , me.alias
            , me.avatar
            , me.description
            , me.contact_id
            , chat::contact::type_enum::person
        }};

        typename serializer_type::output_packet_type out {};
        out << c;
        dispatch_data(addressee, CONTACT_MESSAGE, out.data());
    }

    /**
     * Dispatch group contact and list of group members.
     * Used when group created or updated localy.
     */
    void dispatch_group (contact::id addressee, contact::id group_id) const
    {
        // Skip own contact
        if (addressee == my_contact().contact_id)
            return;

        auto g = _contact_manager->get(group_id);

        if (!is_valid(g)) {
            failure(tr::f_("group not found by id {}", group_id));
            return;
        }

        // Send group contact
        protocol::contact_credentials c {{
              g.contact_id
            , g.alias
            , g.avatar
            , g.description
            , g.creator_id
            , chat::contact::type_enum::group
        }};

        {
            typename serializer_type::output_packet_type out {};
            out << c;
            dispatch_data(addressee, CONTACT_MESSAGE, out.data());
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
            dispatch_data(addressee, MEMBERS_MESSAGE, out.data());
        }
    }

    /**
     * Dispatch group removed message.
     * Used when group removed localy.
     */
    void dispatch_group_removed (contact::id addressee, contact::id group_id) const
    {
        // Skip own contact
        if (addressee == my_contact().contact_id)
            return;

        protocol::group_members gm;
        gm.group_id = group_id;

        typename serializer_type::output_packet_type out {};
        out << gm;

        dispatch_data(addressee, MEMBERS_MESSAGE, out.data());
    }

    /**
     * Dispatch self created group contacts and list of members.
     */
    void dispatch_self_created_groups (contact::id addressee) const
    {
        auto my_contact_id = my_contact().contact_id;

        for_each_contact([this, addressee, my_contact_id] (contact::contact const & c) {
            auto self_created_group = (c.type == contact::type_enum::group)
                && (c.creator_id == my_contact_id);

            if (self_created_group)
                dispatch_group(addressee, c.contact_id);
        });
    }

    /**
     * Dispatch messages limited by @a max_count that not dispatched or not delivered
     * (received on opponent side) status.
     */
    void dispatch_delayed_messages (contact::id addressee_id, int max_count = -1)
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
    void process_received_data (contact::id author, std::string const & data)
    {
        typename serializer_type::input_packet_type in {data};
        protocol::packet_type_enum packet_type;
        in >> packet_type;

        switch (packet_type) {
            case protocol::packet_type_enum::contact_credentials: {
                protocol::contact_credentials c;
                in >> c;

                switch (c.contact.type) {
                    case contact::type_enum::person: {
                        contact::person p;
                        p.contact_id = c.contact.contact_id;
                        p.alias = std::move(c.contact.alias);
                        p.avatar = std::move(c.contact.avatar);
                        p.description = std::move(c.contact.description);

                        /*auto id = */update_or_add(std::move(p));
                        break;
                    }

                    case contact::type_enum::group: {
                        contact::group g;
                        g.contact_id = c.contact.contact_id;
                        g.creator_id = c.contact.creator_id;
                        g.alias = std::move(c.contact.alias);
                        g.avatar = std::move(c.contact.avatar);
                        g.description = std::move(c.contact.description);

                        /*auto id = */update_or_add(std::move(g));
                        break;
                    }

                    case contact::type_enum::channel:
                        // TODO Implement
                        break;

                    default:
                        failure(fmt::format("Unsupported contact type: {}"
                            , static_cast<int>(c.contact.type)));
                        break;
                }

                break;
            }

            case protocol::packet_type_enum::group_members: {
                protocol::group_members gm;
                in >> gm;

                if (gm.members.empty()) {
                    // Group removed or contact has been removed from group.
                    // So remove group locally.
                    //gref.remove_all_members();
                    remove(gm.group_id);
                } else {
                    auto gref = _contact_manager->gref(gm.group_id);

                    if (!gref) {
                        failure(tr::f_("group members received but group contact"
                            " not found: {}", gm.group_id));
                        break;
                    }

                    auto diffs = gref.update(gm.members);

                    if (group_members_updated)
                        group_members_updated(gm.group_id
                            , std::move(diffs.added)
                            , std::move(diffs.removed));
                }

                break;
            }

            case protocol::packet_type_enum::original_message: {
                protocol::original_message m;
                in >> m;
                auto conv = this->conversation(m.author_id);

                if (conv) {
                    try {
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
                        if (message_received)
                            message_received(m.author_id, m.message_id);
                    } catch (error ex) {
                        failure(fmt::format("Bad/corrupted content in "
                            "incoming message {} from: {}"
                            , m.message_id
                            , author));
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
    void dispatched (contact::id addressee
        , message::id message_id
        , pfs::utc_time_point dispatched_time)
    {
        // This is a service message, ignore it
        if (message_id == CONTACT_MESSAGE || message_id == MEMBERS_MESSAGE)
            return;

        auto conv = this->conversation(addressee);

        if (conv) {
            conv.mark_dispatched(message_id, dispatched_time);

            if (message_dispatched)
                message_dispatched(addressee, message_id, dispatched_time);
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
    bool dispatch_received_notification (contact::id addressee_id
        , message::id message_id
        , pfs::utc_time_point received_time)
    {
        protocol::delivery_notification m;
        m.message_id     = message_id;
        m.addressee_id   = my_contact().contact_id; // Addressee is me
        m.delivered_time = received_time;

        typename serializer_type::output_packet_type out {};
        out << m;
        auto success = dispatch_data(addressee_id, message_id, out.data());
        return success;
    }

    /**
     * Process notification of message delivered to @a addressee.
     */
    void process_delivered_notification (contact::id addressee
        , message::id message_id
        , pfs::utc_time_point delivered_time)
    {
        auto conv = this->conversation(addressee);

        if (conv) {
            conv.mark_delivered(message_id, delivered_time);

            if (message_delivered)
                message_delivered(addressee, message_id, delivered_time);
        }
    }

    /**
     * Process notification of message read by @a addressee.
     */
    void process_read_notification (contact::id addressee_id
        , message::id message_id
        , pfs::utc_time_point read_time)
    {
        auto conv = this->conversation(addressee_id);

        if (conv) {
            conv.mark_read(message_id, read_time);

            if (message_read)
                message_read(addressee_id, message_id, read_time);
        }
    }
};

template <typename ContactManagerBackend
    , typename MessageStoreBackend
    , typename FileCacheBackend
    , typename SerializerBackend>
message::id const
messenger<ContactManagerBackend, MessageStoreBackend, FileCacheBackend, SerializerBackend>
    ::CONTACT_MESSAGE {"00000000000000000000000001"_uuid};

template <typename ContactManagerBackend
    , typename MessageStoreBackend
    , typename FileCacheBackend
    , typename SerializerBackend>
message::id const
messenger<ContactManagerBackend, MessageStoreBackend, FileCacheBackend, SerializerBackend>
    ::MEMBERS_MESSAGE {"00000000000000000000000002"_uuid};

} // namespace chat
