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
#include "delivery_manager.hpp"
#include "error.hpp"
#include "message.hpp"
#include "message_store.hpp"
#include "pfs/emitter.hpp"
#include "pfs/fmt.hpp"
#include <memory>
#include <vector>
#include <cassert>

namespace chat {

//
//     ------------------------      -----------------------
//     |    Contact manager   |      |Message store manager|
//     ------------------------      -----------------------
//                 ^                           ^
//                 |                           |
//                 |                           |
//                 v                           v
//         --------------------------------------------
//         |                                          |
//         |           M E S S E N G E R              |
//         |                                          |
//         --------------------------------------------
//                                             ^
//                                             |
//                                             |
//                                             v
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
template <typename ContactManagerBackend
    , typename MessageStoreBackend
    , typename DeliveryManagerBackend
    , template <typename ...Args> class Emitter = pfs::emitter_mt>
class messenger
{
public:
    using contact_manager_type  = contact_manager<ContactManagerBackend>;
    using message_store_type    = message_store<MessageStoreBackend>;
    using delivery_manager_type = delivery_manager<DeliveryManagerBackend>;
//     using icon_library_type    = typename PersistentStorageAPI::icon_library_type;
//     using media_cache_type     = typename PersistentStorageAPI::media_cache_type;

    using conversation_type = typename message_store_type::conversation_type;

private:
    std::unique_ptr<contact_manager_type>  _contact_manager;
    std::unique_ptr<message_store_type>    _message_store;
    std::unique_ptr<delivery_manager_type> _delivery_manager;
    contact::id_generator _contact_id_generator;
    message::id_generator _message_id_generator;

    typename delivery_manager_type::message_dispatched_callback _message_dispatched;
    typename delivery_manager_type::message_delivered_callback  _message_delivered;
    typename delivery_manager_type::message_read_callback       _message_read;

public: // signals
    mutable Emitter<error const &> failure;
    mutable Emitter<contact::contact_id /*addressee*/
            , message::message_id /*message_id*/
            , pfs::utc_time_point /*dispatched_time*/> message_dispatched;

    mutable Emitter<contact::contact_id /*addressee*/
            , message::message_id /*message_id*/
            , pfs::utc_time_point /*delivered_time*/> message_delivered;

    mutable Emitter<contact::contact_id /*addressee*/
            , message::message_id /*message_id*/
            , pfs::utc_time_point /*read_time*/> message_read;

public:
    messenger (std::unique_ptr<contact_manager_type> && contact_manager
        , std::unique_ptr<message_store_type> && message_store
        , std::unique_ptr<delivery_manager_type> && delivery_manager)
        : _contact_manager(std::move(contact_manager))
        , _message_store(std::move(message_store))
        , _delivery_manager(std::move(delivery_manager))
    {
        // Set default failure callback
        failure.connect([] (error const & err) {
            fmt::print(stderr, "ERROR: {}\n", err.what());
        });

        _message_dispatched = [this] (contact::contact_id addressee
            , message::message_id message_id
            , pfs::utc_time_point dispatched_time) {

            auto conv = this->conversation(addressee);

            if (conv) {
                conv.mark_dispatched(message_id, dispatched_time);
                this->message_dispatched(addressee, message_id, dispatched_time);
            }
        };

        _message_delivered = [this] (contact::contact_id addressee
            , message::message_id message_id
            , pfs::utc_time_point delivered_time) {

            auto conv = this->conversation(addressee);

            if (conv) {
                conv.mark_delivered(message_id, delivered_time);
                this->message_delivered(addressee, message_id, delivered_time);
            }
        };

        _message_read = [this] (contact::contact_id addressee
            , message::message_id message_id
            , pfs::utc_time_point read_time) {

            auto conv = this->conversation(addressee);

            if (conv) {
                conv.mark_read(message_id, read_time);
                this->message_read(addressee, message_id, read_time);
            }
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
            && *_message_store
            && *_delivery_manager;
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
        error err;

        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(fmt::format("attempt to get members of non-existent group: #{}"
                , to_string(group_id)));
            return std::vector<contact::contact>{};
        }

        return group_ref.members(& err);
    }

    /**
     * Count of contacts in specified group.
     */
    std::size_t members_count (contact::contact_id group_id) const
    {
        auto group_ref = _contact_manager->gref(group_id);

        if (!group_ref) {
            failure(fmt::format("attempt to get members count of non-existent group: #{}"
                , to_string(group_id)));
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

    template <typename F>
    void for_each_contact (F && f) const
    {
        _contact_manager->for_each(std::forward<F>(f));
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
            failure(fmt::format("attempt to add member into non-existent group: #{}"
                , to_string(group_id)));
            return false;
        }

        return group_ref.add_member(member_id);
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
            addressee_id = contact::contact_id{};
            return conversation_type{};
        }

        return _message_store->conversation(addressee_id);
    }

    // TODO
//     auto unread_messages_count (contact::contact_id id) const -> std::size_t
//     {
//         auto conv = _message_store->conversation(id);
//         return conv.unread_messages_count();
//     }

    /**
     * Dispatch message (original or edited)
     */
    void dispatch (contact::contact_id addressee
        , message::message_credentials const & msg)
    {
        auto result = _delivery_manager->dispatch(addressee
            , msg
            , _message_dispatched
            , _message_delivered
            , _message_read);
    }

    void wipe ()
    {
        _contact_manager->wipe();
        _message_store->wipe();
    }
};

} // namespace chat
