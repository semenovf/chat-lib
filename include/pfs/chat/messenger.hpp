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
    mutable Emitter<std::string const &> failure;
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
        _message_dispatched = [this] (contact::contact_id addressee
            , message::message_id message_id
            , pfs::utc_time_point dispatched_time) {

            auto conv = this->conversation(addressee);

            if (conv) {
                error err;
                conv.mark_dispatched(message_id, dispatched_time, & err);

                if (err) {
                    this->failure(err.what());
                } else {
                    this->message_dispatched(addressee, message_id, dispatched_time);
                }
            }
        };

        _message_delivered = [this] (contact::contact_id addressee
            , message::message_id message_id
            , pfs::utc_time_point delivered_time) {

            auto conv = this->conversation(addressee);

            if (conv) {
                error err;
                conv.mark_delivered(message_id, delivered_time, & err);

                if (err) {
                    this->failure(err.what());
                } else {
                    this->message_delivered(addressee, message_id, delivered_time);
                }
            }
        };

        _message_read = [this] (contact::contact_id addressee
            , message::message_id message_id
            , pfs::utc_time_point read_time) {

            auto conv = this->conversation(addressee);

            if (conv) {
                error err;
                conv.mark_read(message_id, read_time, & err);

                if (err) {
                    this->failure(err.what());
                } else {
                    this->message_read(addressee, message_id, read_time);
                }
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
    contact::contact my_contact () const
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
     * Total count of contacts with specified @a type.
     */
    std::size_t contacts_count (contact::type_enum type) const
    {
        return _contact_manager->count(type);
    }

    /**
     * Add contact.
     *
     * @return Identifier of just added contact or @c chat::contact::contact_id{}
     *         on error.
     */
    contact::contact_id add (contact::contact c)
    {
        error err;

        if (c.id == contact::contact_id{}) {
            c.id = _contact_id_generator.next();
        }

        auto rc = _contact_manager->add(c, & err);

        // Error
        if (rc < 0 ) {
            failure(err.what());
            return false;
        }

        return rc > 0 ? c.id : contact::contact_id{};
    }

    /**
     * Add person contact.
     *
     * @return Identifier of just added contact or @c chat::contact::contact_id{}
     *         on error.
     */
    contact::contact_id add (contact::person const & p)
    {
        contact::contact c { _contact_id_generator.next()
            , p.alias, p.avatar, p.description, contact::type_enum::person};
        return add(c);
    }

    /**
     * Add group contact.
     *
     * @return Identifier of just added contact or @c chat::contact::contact_id{}
     *         on error.
     */
    contact::contact_id add (contact::group const & g)
    {
        contact::contact c {_contact_id_generator.next()
            , g.alias, g.avatar, g.description, contact::type_enum::group};
        return add(c);
    }

    /**
     * Add channel contact.
     *
     * @return Identifier of just added contact or @c chat::contact::contact_id{}
     *         on error.
     */
    contact::contact_id add (contact::channel const & ch)
    {
        contact::contact c {_contact_id_generator.next()
            , ch.alias, ch.avatar, ch.description, contact::type_enum::channel};
        return add(c);
    }

    /**
     * Update contact.
     *
     * @return @c true if contact updated successfully, @c false on error or
     *         contact does not exist.
     */
    bool update (contact::contact const & c)
    {
        error err;
        auto rc = _contact_manager->update(c, & err);

        // Error
        if (rc < 0) {
            failure(err.what());
            return false;
        }

        return rc > 0 ? true : false;
    }

    bool update (contact::person const & p)
    {
        contact::contact c {
              p.id
            , p.alias
            , p.avatar
            , p.description
            , contact::type_enum::person
        };

        return update(c);
    }

    bool update (contact::group const & g)
    {
        contact::contact c {
              g.id
            , g.alias
            , g.avatar
            , g.description
            , contact::type_enum::group};
        return update(c);
    }

    /**
     * Removes contact.
     *
     * @details If @a id is a group then group contact and all memberships
     *          will be removed. If @a id is a person contact membership will
     *          be removed in case of group participation.
     */
    bool remove (contact::contact_id id)
    {
        error err;
        auto rc = _contact_manager->remove(id, & err);

        // Error
        if (rc < 0) {
            failure(err.what());
            return false;
        }

        return rc > 0 ? true : false;
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact>
    contact (contact::contact_id id) const noexcept
    {
        error err;
        auto contact = _contact_manager->get(id, & err);

        if (err) {
            failure(err.what());
            return pfs::nullopt;
        }

        return contact;
    }

    /**
     * Get contact by offset.
     */
    pfs::optional<contact::contact>
    contact (int offset) const noexcept
    {
        error err;
        auto contact = _contact_manager->get(offset, & err);

        if (err) {
            failure(err.what());
            return pfs::nullopt;
        }

        return contact;
    }

    template <typename F>
    void for_each_contact (F && f) const
    {
        error err;
        _contact_manager->for_each(f, & err);

        if (err) {
            failure(err.what());
        }
    }

    /**
     * Adds member specified by @a member_id to the group specified by @a group_id.
     *
     * @return @c false on error or @c true if contact added was successfully
     *         or it is already a member of the specified group.
     */
    bool add_member (contact::contact_id group_id, contact::contact_id member_id)
    {
        error err;
        auto group_ref = _contact_manager->groups()->ref(group_id);
        auto rc = group_ref->add_member(member_id, & err);

        // Error
        if (rc < 0) {
            failure(err.what());
            return false;
        }

        return rc > 0 ? true : false;
    }

    /**
     * Checks if contact @a member_id is the member of group @a group_id.
     */
    bool is_member_of (contact::contact_id member_id, contact::contact_id group_id) const
    {
        auto group_ref = _contact_manager->groups()->ref(group_id);
        return group_ref.is_member_of(member_id);
    }

    conversation_type conversation (contact::contact_id addressee_id) const
    {
        // Check for contact exists
        auto opt = contact(addressee_id);

        if (!opt) {
            // Invalid addressee ID to generate invalid conversation
            addressee_id = contact::contact_id{};
        }

        error err;
        auto result = _message_store->conversation(addressee_id, & err);

        if (err)
            failure(err.what());

        return result;
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
        error err;

        auto result = _delivery_manager->dispatch(addressee
            , msg
            , _message_dispatched
            , _message_delivered
            , _message_read
            , & err);

        if (!result) {
            failure(err.what());
        }
    }

    void wipe ()
    {
        _contact_manager->wipe();
        _message_store->wipe();
    }
};

} // namespace chat
