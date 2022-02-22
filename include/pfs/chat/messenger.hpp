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

public: // signals
    mutable Emitter<std::string const &> failure;

//     /**
//      * @function void dispatch (contact::contact_id addressee, std::vector<char> const & data)
//      *
//      * @brief Dispatch message signal.
//      *
//      * @param addressee Message addressee.
//      * @param data      Serialized message.
//      */
//     pfs::emitter_mt<contact::contact_id, std::vector<char> const &> dispatch;

    //pfs::emitter_mt<message::message_id> dispatched;

private:
    bool add_contact_helper (contact::contact const & c, bool force_update)
    {
        error err;
        auto rc = _contact_manager->contacts()->add(c, & err);

        if (rc > 0) {
            // Contact added successfully
            return true;
        } else if (rc == 0) {
            // Contact already exists

            if (force_update) {
                rc = _contact_manager->contacts()->update(c, & err);

                if (rc > 0) {
                    return true;
                } else if (rc == 0) {
                    ; // Unexpected state (contact existence checked before)
                } else {
                    failure(err.what());
                }
            } else {
                failure(fmt::format("contact already exists: {} (#{})"
                    , c.alias
                    , to_string(c.id)));
            }
        } else {
            // Error
            failure(err.what());
        }

        return false;
    }

public:
    messenger (std::unique_ptr<contact_manager_type> && contact_manager
        , std::unique_ptr<message_store_type> && message_store
        , std::unique_ptr<delivery_manager_type> && delivery_manager)
        : _contact_manager(std::move(contact_manager))
        , _message_store(std::move(message_store))
        , _delivery_manager(std::move(delivery_manager))
    {}

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
        return _contact_manager->contacts()->count();
    }

    /**
     * Total count of contacts with specified @a type.
     */
    std::size_t contacts_count (contact::type_enum type) const
    {
        return _contact_manager->contacts()->count(type);
    }

    /**
     * Add contact
     */
    auto add_contact (contact::contact const & c) -> bool
    {
        return add_contact_helper(c, false);
    }

    /**
     * Add or update contact
     */
    auto add_or_update_contact (contact::contact const & c) -> bool
    {
        return add_contact_helper(c, true);
    }

    /**
     * Get contact by @a id.
     */
    pfs::optional<contact::contact>
    get_contact (contact::contact_id id) const noexcept
    {
        error err;
        auto contact = _contact_manager->contacts()->get(id, & err);

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
    get_contact (int offset) const noexcept
    {
        error err;
        auto contact = _contact_manager->contacts()->get(offset, & err);

        if (err) {
            failure(err.what());
            return pfs::nullopt;
        }

        return contact;

        return _contact_manager->contacts().get(offset);
    }

    template <typename F>
    void for_each_contact (F && f) const
    {
        auto contacts = _contact_manager->contacts();

        for (auto c: contacts) {
            f(c);
        }
    }

    conversation_type conversation (contact::contact_id addressee_id) const
    {
        // Check for contact exists
        auto opt = get_contact(addressee_id);

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

        auto message_dispatched = [this] (contact::contact_id addressee
            , message::message_id message_id
            , pfs::utc_time_point dispatched_time) {

            auto conv = this->conversation(addressee);

            if (conv) {
                error err;
                conv.mark_dispatched(message_id, dispatched_time, & err);

                if (err) {
                    this->failure(err.what());
                }
            }
        };

        auto result = _delivery_manager->dispatch(addressee
            , msg
            , message_dispatched
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
