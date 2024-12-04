////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.11.17 Initial version.
//      2024.12.02 Started V2.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "activity_manager.hpp"
#include "contact.hpp"
#include "contact_manager.hpp"
#include "error.hpp"
// #include "file.hpp"
#include "file_cache.hpp"
// #include "message.hpp"
#include "message_store.hpp"
// #include "protocol.hpp"
#include "primal_serializer.hpp"
#include "callback_traits/function.hpp"
// #include "pfs/assert.hpp"
// #include "pfs/fmt.hpp"
// #include "pfs/i18n.hpp"
// #include "pfs/log.hpp"
// #include "pfs/numeric_cast.hpp"
// #include <algorithm>
// #include <functional>
// #include <iterator>
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
//  |               ^                           ^                |
//  |               |                           |                |
//  |               v                           v                |
//  |       ------------------       -----------------------     |
//  |       |Activity manager|<------|   Delivery manager  |     |
//  |       ------------------       -----------------------     |
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
//     |                  |      3. chat ID            |              |
//     | dispatch_message |      4. creation time      |              |
//     |----------------->| (1)  5. content            |              |
//     |                  |--------------------------->|          (1')|
//     |                  |                            |------------->|
//     |                  |      1. message ID         |              |
//     |                  |      2. addressee ID       |              |
//     |                  |      3. chat ID'           |           (2)|
//     |                  | (2') 4. delivered time     |<-------------|
//     |                  |<---------------------------|              |
//     |                  |                            |              |
//     |                  |      1. message ID         |              |
//     |                  |      2. addressee ID       |              |
//     |                  |      3. chat ID'           |           (3)|     (3")
//     |                  | (3') 4. read time          |<-------------|-------->
//     |                  |<---------------------------|              |
//     |                  |                            |              |
//     |                  |      1. message ID         |              |
//     |                  |      2. author ID          |              |
//     |                  |      3. chat ID            |              |
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
// chat ID  -> Author or group ID
// chat ID' -> Addressee or group ID
////////////////////////////////////////////////////////////////////////////////
/**
 * Terms
 *
 * -# @c Addressee Side (person) of the chat that received the message.
 *          Synonym for @c Receiver.
 * -# @c Addresser Side (person) of the chat that send the message.
 *          Synonym for @c Sender.
 * -# @c Author Side (person) of the chat that create and send the message.
 * -# @c Chat Chat (dialogue, conversation, talk) as is. Can be personal chat,
 *          group chat or channel. Chat associates with two (personal) or more
 *          (group chat or channel) sides.
 * -# @c Chat @c credentials Data that dsscribes chat.
 * -# @c Contact Is a synonym for chat credentials.
 * -# @c Message The unit of the chat. Has author (creator) and destination
 *          (another side of the chat).
 * -# @c Opponent Opposite side of the chat. Can be synonym for
 *          @c Addressee / @c Receiver.
 * -# @c Receiver Side (person) of the chat that received the message.
 *          Synonym for @c Addressee.
 * -# @c Sender Side (person) of the chat that send the message.
 */
////////////////////////////////////////////////////////////////////////////////

template <typename ContactManagerStorage
    , typename MessageStoreStorage = ContactManagerStorage
    , typename ActivityManagerStorage = ContactManagerStorage
    , typename FileCacheStorage = ContactManagerStorage
    , typename Serializer = primal_serializer<pfs::endian::network>
    , typename CallbackTraits = function_callbacks>
class messenger: public CallbackTraits
{
public:
    using contact_type = contact::contact;
    using person_type  = contact::person;
    using group_type   = contact::group;
    using contact_manager_type  = contact_manager<ContactManagerStorage>;
    using message_store_type    = message_store<MessageStoreStorage>;
    using activity_manager_type = activity_manager<ActivityManagerStorage>;
    using file_cache_type       = file_cache<FileCacheStorage>;
    using serializer_type       = Serializer;
    using chat_type = typename message_store_type::chat_type;

private:
    contact_manager_type  _contact_manager;
    message_store_type    _message_store;
    activity_manager_type _activity_manager;
    file_cache_type       _file_cache;
    contact::id_generator _contact_id_generator;
    message::id_generator _message_id_generator;

public:
    messenger (contact_manager_type && contact_manager
        , message_store_type && message_store
        , activity_manager_type && activity_manager
        , file_cache_type && file_cache)
        : _contact_manager(std::move(contact_manager))
        , _message_store(std::move(message_store))
        , _activity_manager(std::move(activity_manager))
        , _file_cache(std::move(file_cache))
    {}

    messenger (messenger &&) = default;
    ~messenger () = default;

    messenger (messenger const &) = delete;
    messenger & operator = (messenger const &) = delete;
    messenger & operator = (messenger &&) = delete;

    /**
     * Local contact.
     */
    contact::person my_contact () const
    {
        return _contact_manager.my_contact();
    }

    /**
     * Changes @a alias for my contact.
     */
    void change_my_alias (std::string alias)
    {
        _contact_manager.change_my_alias(std::move(alias));
    }

    /**
     * Changes @a avatar for my contact.
     */
    void change_my_avatar (std::string avatar)
    {
        _contact_manager.change_my_avatar(std::move(avatar));
    }

    /**
     * Changes description for my contact.
     */
    void change_my_desc (std::string desc)
    {
        _contact_manager.change_my_desc(std::move(desc));
    }

////////////////////////////////////////////////////////////////////////////////
// Group contact specific methods
////////////////////////////////////////////////////////////////////////////////
    /**
     * Adds member specified by @a member_id to the group specified by @a group_id.
     *
     * @return @c true if member successfully added to group @a group_id, or @c false if
     *         if member already exists.
     *
     * @throw errc::group_not_found
     */
    bool add_member (contact::id group_id, contact::id member_id)
    {
        auto group_ref = _contact_manager.gref(group_id);

        if (!group_ref)
            throw error { errc::group_not_found, to_string(group_id) };

        return group_ref.add_member(member_id);
    }

    /**
     * Removes member specified by @a member_id from the group specified by @a group_id.
     */
    void remove_member (contact::id group_id, contact::id member_id, std::error_code & ec)
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref)
            throw error { errc::group_not_found, to_string(group_id) };

        group_ref.remove_member(member_id);
    }

    /**
     * Removes all members from the group specified by @a group_id.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found by @a group_id.
     */
    void remove_all_members (contact::id group_id, std::error_code & ec) noexcept
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref)
            throw error { errc::group_not_found, to_string(group_id) };

        group_ref.remove_all_members();
    }

    /**
     * Gets members of the specified group. Returns empty vector on error.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found by @a group_id.
     */
    std::vector<contact::contact> members (contact::id group_id) const
    {
        auto group_ref = _contact_manager.gref(group_id);

        if (!group_ref)
            throw error {errc::group_not_found, to_string(group_id)};

        return group_ref.members();
    }

    /**
     * Get member identifiers of the specified group.
     *
     * @throw chat::error{errc::group_not_found} if conversation group not found by @a group_id.
     */
    std::vector<contact::id> member_ids (contact::id group_id) const
    {
        auto group_ref = _contact_manager.gref(group_id);

        if (!group_ref)
            throw error {errc::group_not_found, to_string(group_id)};

        return group_ref.member_ids();
    }

    /**
     * Count of contacts in specified group.
     */
    inline std::size_t members_count (contact::id group_id) const
    {
        return _contact_manager.members_count(group_id);
    }

    /**
     * Checks if contact @a member_id is the member of group @a group_id.
     */
    bool is_member_of (contact::id group_id, contact::id member_id) const
    {
        auto group_ref = _contact_manager.gref(group_id);

        if (!group_ref)
            throw error {errc::group_not_found, to_string(group_id)};

        return group_ref.is_member_of(member_id);
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
    add (ConcreteContactType && c)
    {
        if (c.contact_id == contact::id{})
            c.contact_id = _contact_id_generator.next();

        auto contact_id = c.contact_id;

        if (_contact_manager.add(std::move(c))) {
            this->contact_added(contact_id);
            return contact_id;
        }

        return contact::id{};
    }

    /**
     * Update contact (personal or group).
     *
     * @return @c true if contact updated successfully or @c false otherwise.
     */
    template <typename ConcreteContactType>
    typename std::enable_if<std::is_same<ConcreteContactType, contact::person>::value
        || std::is_same<ConcreteContactType, contact::group>::value, bool>::type
    update (ConcreteContactType && c)
    {
        auto contact_id = c.contact_id;

        if (_contact_manager.update(std::move(c))) {
            this->contact_updated(contact_id);
            return true;
        }

        return false;
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
    update_or_add (ConcreteContactType && p)
    {
        if (p.contact_id == contact::id{}) {
            p.contact_id = _contact_id_generator.next();
            return add(std::move(p));
        }

        if (!update(ConcreteContactType{p})) {
            auto id = add(std::move(p));
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
        _contact_manager.remove(id);
        clear_chat(id);
        this->contact_removed(id);
    }

    chat_type open_chat (contact::id chat_id)
    {
        // Check for contact exists
        auto c = _contact_manager.get(chat_id);

        if (!is_valid(c))
            return chat_type{};

        auto result = _message_store.open_chat(chat_id);

        result.cache_outgoing_local_file = [this, chat_id] (message::id message_id
                , std::int16_t attachment_index, pfs::filesystem::path const & path) {
            return _file_cache.cache_outgoing_file(my_contact().contact_id
                , chat_id, message_id, attachment_index, path);
        };

        result.cache_outgoing_custom_file = [this, chat_id] (
                  message::id message_id
                , std::int16_t attachment_index
                , std::string const & uri
                , std::string const & display_name
                , std::int64_t size
                , pfs::utc_time modtime) {
            return _file_cache.cache_outgoing_file(my_contact().contact_id
                , chat_id, message_id, attachment_index, uri
                , display_name, size, modtime);
        };

        return result;
    }

    /**
     * Clears chat messages and attachments
     */
    void clear_chat (contact::id chat_id)
    {
        auto cht = _message_store.open_chat(chat_id);

        if (cht) {
            cht.clear();
            // TODO Implement clear file cache with removing incoming files.
            //_file_cache->clear(chat_id);
        }
    }

    /**
     * Total unread messages count.
     */
    std::size_t unread_message_count ()
    {
        std::size_t result = 0;

        _contact_manager.for_each([this, & result] (contact::contact const & c) {
            auto cht = open_chat(c.contact_id);

            if (cht)
                result += cht.unread_message_count();
        });

        return result;
    }

//     // FIXME DEPRECATED
//     template <typename F>
//     bool transaction (F && op) noexcept
//     {
//         return _contact_manager->transaction(std::forward<F>(op));
//     }

    /**
     * Dispatch message (original or edited) for person or group.
     *
     * @param talk Chat that the message belongs to.
     * @param message_id Message identifier.
     *
     * @throw error{errc::bad_chat_type} Bad chat type.
     */
    void dispatch_message (chat_type const & cht, message::id message_id)
    {
        if (!cht)
            return;

        auto addressee = _contact_manager.get(cht.id());
        auto msg = cht.message(message_id);

        protocol::regular_message m;
        m.message_id = msg->message_id;
        m.author_id  = msg->author_id;
        m.chat_id = contact::is_person(addressee) ? msg->author_id : cht.id();
        m.mod_time   = msg->modification_time;
        m.content    = msg->contents.has_value() ? to_string(*msg->contents) : std::string{};

        typename serializer_type::ostream_type out;
        out << m;
        dispatch_multicast(addressee, out.take());
    }

    /**
     * This is a convenient method for dispatch message
     * (@see messenger::dispatch_message(conversation_type const &, message::id)).
     */
    inline void dispatch_message (contact::id chat_id, message::id message_id)
    {
        dispatch_message(this->open_chat(chat_id), message_id);
    }

    /**
     * Mark received message as read and dispatch read notification to message author or members
     * of chat group.
     *
     * @param chat_id Chat identifier.
     * @param message_id Message identifier.
     * @param read_time Read time in UTC.
     *
     * @throw chat::error{errc::chat_not_found} if conversation not found specified by @a message_address.
     * @throw chat::error{errc::message_not_found} if message not found specified by @a message_address.
     */
    void dispatch_read_notification (contact::id chat_id, message::id message_id, pfs::utc_time_point read_time)
    {
        //
        // Mark incoming message as read
        //
        auto cht = this->open_chat(chat_id);

        if (!cht)
            throw error {errc::chat_not_found, to_string(chat_id)};

        process_read_notification(cht, message_id, read_time);

        //
        // Dispatch message read notification to message sender.
        //
        auto chat_contact = _contact_manager.get(cht.id());

        protocol::read_notification m;
        m.message_id = message_id;
        m.chat_id = contact::is_person(chat_contact) ? my_contact().contact_id : chat_id;
        m.read_time = read_time;

        typename serializer_type::ostream_type out;
        out << m;
        this->dispatch_data(chat_contact.contact_id, out.take());
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
            , me.extra
            , me.contact_id
            , chat_enum::person
        }};

        typename serializer_type::ostream_type out;
        out << c;
        this->dispatch_data(addressee_id, out.take());
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

        auto g = _contact_manager.get(group_id);

        if (!is_valid(g))
            throw error {errc::group_not_found, to_string(group_id)};

        // Send group contact
        protocol::contact_credentials c {{
              g.contact_id
            , g.alias
            , g.avatar
            , g.description
            , g.extra
            , g.creator_id
            , chat_enum::group
        }};

        {
            typename serializer_type::ostream_type out;
            out << c;
            this->dispatch_data(addressee_id, out.take());
        }

        // Send group members
        auto gref = _contact_manager->gref(group_id);
        auto members = gref.members();

        protocol::group_members gm;
        gm.group_id = group_id;

        for (auto const & c: members)
            gm.members.push_back(c.contact_id);

        {
            typename serializer_type::ostream_type out;
            out << gm;
            this->dispatch_data(addressee_id, out.take());
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

        typename serializer_type::ostream_type out;
        out << gm;
        this->dispatch_data(addressee_id, out.take());
    }

    /**
     * Dispatch self created group contacts and list of members.
     */
    void dispatch_self_created_groups (contact::id addressee_id) const
    {
        auto my_contact_id = my_contact().contact_id;

        _contact_manager.for_each([this, addressee_id, my_contact_id] (contact::contact const & c) {
            auto self_created_group = (c.type == chat_enum::group)
                && (c.creator_id == my_contact_id);

            if (self_created_group) {
                if (is_member_of(c.contact_id, addressee_id))
                    dispatch_group(addressee_id, c.contact_id);
            }
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

        typename serializer_type::ostream_type out;
        out << protocol::file_request{file_id};
        this->dispatch_data(addressee_id, out.take());
    }

    void dispatch_file_error (contact::id addressee_id, file::id file_id)
    {
        // Skip own contact
        if (addressee_id == my_contact().contact_id)
            return;

        protocol::file_error m;
        m.file_id = file_id;

        typename serializer_type::ostream_type out;
        out << m;
        this->dispatch_data(addressee_id, out.take());
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
    void process_incoming_data (contact::id addresser_id, char const * data, std::size_t size)
    {
        typename serializer_type::istream_type in {data, size};
        protocol::packet_enum packet_type;
        in >> packet_type;

        switch (packet_type) {
            case protocol::packet_enum::contact_credentials: {
                protocol::contact_credentials cc;
                in >> cc;

                switch (cc.contact.type) {
                    case chat_enum::person: {
                        contact::person p;
                        p.contact_id = cc.contact.contact_id;
                        p.alias = std::move(cc.contact.alias);
                        p.avatar = std::move(cc.contact.avatar);
                        p.description = std::move(cc.contact.description);
                        p.extra = std::move(cc.contact.extra);

                        /*auto id = */update_or_add(std::move(p));
                        break;
                    }

                    case chat_enum::group: {
                        contact::group g;
                        g.contact_id = cc.contact.contact_id;
                        g.creator_id = cc.contact.creator_id;
                        g.alias = std::move(cc.contact.alias);
                        g.avatar = std::move(cc.contact.avatar);
                        g.description = std::move(cc.contact.description);
                        g.extra = std::move(cc.contact.extra);

                        /*auto id = */update_or_add(std::move(g));
                        break;
                    }

                    case chat_enum::channel:
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
                    auto gref = _contact_manager.gref(gm.group_id);

                    if (!gref) {
                        throw error{errc::group_not_found, to_string(gm.group_id)};
                        break;
                    }

                    auto diffs = gref.update(gm.members);
                    this->group_members_updated(gm.group_id, std::move(diffs.added), std::move(diffs.removed));
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
    void commit_incoming_file (file::id file_id, pfs::filesystem::path const & path)
    {
        _file_cache.commit_incoming_file(file_id, path);
    }

    file::optional_credentials incoming_file (file::id file_id) const
    {
        return _file_cache.incoming_file(file_id);
    }

    file::optional_credentials outgoing_file (file::id file_id) const
    {
        return _file_cache.outgoing_file(file_id);
    }

    /**
     * Total list of incoming files (attachments) from specified conversation.
     */
    std::vector<file::credentials> incoming_files (contact::id chat_id) const
    {
        return _file_cache.incoming_files(chat_id);
    }

    /**
     * Total list of outgoing files (attachments) for specified conversation.
     */
    std::vector<file::credentials> outgoing_files (contact::id chat_id) const
    {
        return _file_cache.outgoing_files(chat_id);
    }

    /**
     * Erases all person contacts, groups and channels.
     */
    void clear_contacts ()
    {
        _contact_manager.clear();
    }

    /**
     * Erases all messages.
     */
    void clear_messages ()
    {
        _message_store.clear();
    }

    /**
     * Erases file cache.
     */
    void clear_file_cache ()
    {
        _file_cache->clear();
    }

    /**
     * Clear all person contacts, groups, channels and all messages, and file cache.
     */
    void clear_all ()
    {
        _contact_manager.clear();
        _message_store.clear();
        _activity_manager.clear();
        _file_cache.clear();
    }

    activity_manager_type const & amanager () const noexcept
    {
        return _activity_manager;
    }

    contact_manager_type const & cmanager () const noexcept
    {
        return _contact_manager;
    }

    message_store_type const & mstore () const noexcept
    {
        return _message_store;
    }

private:
    // Can be considered that `dispatch_data` is analog to `dispatch_unicast`.
    void dispatch_multicast (contact::contact const & addressee
        , typename serializer_type::output_archive_type const & data)
    {
        switch (addressee.type) {
            case chat_enum::person:
                this->dispatch_data(addressee.contact_id, data);
                break;

            case chat_enum::group: {
                auto group_ref = _contact_manager.gref(addressee.contact_id);
                auto member_ids = group_ref.member_ids();

                for (auto const & member_id: member_ids) {
                    if (member_id != my_contact().contact_id)
                        this->dispatch_data(member_id, data);
                }

                break;
            }

            case chat_enum::channel:
                // TODO Unsupported yet
                break;

            default:
                throw error{errc::bad_conversation_type};
                break;
        }
    }

    /**
     * @throw chat::error{errc::chat_not_found} if specified in message
     *        @a m conversation not found.
     */
    void process_regular_message (protocol::regular_message const & m)
    {
        auto cht = this->open_chat(m.chat_id);

        if (!cht)
            throw error {errc::chat_not_found, to_string(m.chat_id)};

        // Can throw when bad/corrupted content in incoming message
        message::content content{m.content};

        // Search content for attachments and cache their credentials in the
        // `file_cache`
        for (std::size_t i = 0, count = content.count(); i < count; i++) {
            auto cc = content.at(i);
            auto att = content.attachment(i);

            // Attachment really
            if (!att.name.empty()) {
                _file_cache.reserve_incoming_file(att.file_id, m.author_id
                    , m.chat_id, m.message_id, pfs::numeric_cast<std::int16_t>(i)
                    , att.name, att.size, cc.mime);
            }
        }

        cht.save_incoming(m.message_id, m.author_id, m.mod_time, to_string(content));

        auto received_time = pfs::current_utc_time_point();
        cht.mark_received(m.message_id, received_time);

        // Send notification
        dispatch_delivery_notification(m.author_id, cht.id(), m.message_id, received_time);

        // Notify message received
        this->message_received(m.author_id, m.chat_id, m.message_id);
    }

    /**
     * Dispatch delivery (received by addressee) notification (to author only).
     */
    void dispatch_delivery_notification (contact::id author_id, contact::id chat_id
        , message::id message_id, pfs::utc_time_point received_time)
    {
        auto chat_contact = _contact_manager.get(chat_id);

        if (!is_valid(chat_contact))
            throw error{errc::contact_not_found, to_string(chat_id)};

        auto addressee = _contact_manager.get(author_id);

        if (!is_valid(addressee))
            throw error{errc::contact_not_found, to_string(addressee.contact_id)};

        protocol::delivery_notification m;
        m.message_id      = message_id;
        m.chat_id = contact::is_person(chat_contact) ? my_contact().contact_id : chat_id;
        m.delivered_time  = received_time;

        typename serializer_type::ostream_type out;
        out << m;
        dispatch_multicast(addressee, out.take());
    }

    /**
     * Process notification of message delivered to addressee (receiver).
     *
     * @throw chat::error{errc::chat_not_found} if specified in
     *        notification @a m conversation not found.
     */
    void process_delivered_notification (protocol::delivery_notification const & m)
    {
        auto cht = open_chat(m.chat_id);

        if (!cht)
            throw error {errc::chat_not_found, to_string(m.chat_id)};

        cht.mark_delivered(m.message_id, m.delivered_time);
        this->message_delivered(m.chat_id, m.message_id, m.delivered_time);
    }

    inline void process_read_notification (chat_type & cht, message::id message_id
        , pfs::utc_time read_time)
    {
        cht.mark_read(message_id, read_time);
        this->message_read(cht.id(), message_id, read_time);
    }

    /**
     * Process notification of message read by addressee (receiver).
     *
     * @throw chat::error{errc::chat_not_found} if specified in
     *        notification @a m conversation not found.
     */
    void process_read_notification (protocol::read_notification const & m)
    {
        auto cht = open_chat(m.chat_id);

        if (!cht) {
            throw error {errc::chat_not_found, to_string(m.chat_id)};
        }

        process_read_notification(cht, m.message_id, m.read_time);
    }

    /**
     * Process file request.
     */
    void process_file_request (contact::id addresser_id, protocol::file_request const & m)
    {
        auto fc = _file_cache.outgoing_file(m.file_id);

        if (fc) {
            this->dispatch_file(addresser_id, fc->file_id, fc->abspath);
        } else {
            // File not found in cache by specified ID.
            dispatch_file_error(addresser_id, m.file_id);
        }
    }

    void process_file_error (contact::id addresser_id, protocol::file_error const & m)
    {
        this->on_file_error(addresser_id, m.file_id);
    }
};

} // namespace chat
