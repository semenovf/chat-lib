////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2023.05.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "contact.hpp"
#include "pfs/unicode/search.hpp"
#include "pfs/unicode/utf8_iterator.hpp"
#include <cstdint>
#include <vector>

namespace chat {

class search_flags
{
public:
    enum flag: std::uint32_t {
          ignore_case     = 1 << 0
        , alias_field     = 1 << 1
        , desc_field      = 1 << 2

        , text_content    = 1 << 3
        , attachment_name = 1 << 4
    };

    std::uint32_t value;

public:
    search_flags (std::uint32_t val) : value(val) {}
    search_flags & operator = (std::uint32_t val) { value = val; return *this; }

    search_flags (search_flags const & other) = default;
    search_flags (search_flags && other) = default;
    search_flags & operator = (search_flags const & other) = default;
    search_flags & operator = (search_flags && other) = default;
};

template <typename ContactList>
class contacts_searcher
{
    using utf8_input_iterator = pfs::unicode::utf8_input_iterator<std::string::const_iterator>;

public:
    struct match_spec
    {
        contact::id contact_id;
        std::size_t index; // first match position
        std::size_t count; // number of matches
    };

    struct match_item
    {
        contact::id contact_id;
        search_flags::flag field;      // field that matches search pattern (alias_field or desc_field)
        pfs::unicode::match_item m;
    };

    struct search_result
    {
        std::vector<match_spec> sp;
        std::vector<match_item> m;
    };

private:
    ContactList const * _pcl {nullptr};

public:
    contacts_searcher (ContactList & cl) : _pcl(& cl) {}

private:
    static void search_all_helper (std::string const & s, std::string const & pattern
        , bool ignore_case, std::function<void(pfs::unicode::match_item const &)> && f)
    {
        auto first = utf8_input_iterator::begin(s.begin(), s.end());
        auto s_first = utf8_input_iterator::begin(pattern.begin(), pattern.end());

        pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
            , ignore_case, std::move(f));
    }

public:
    /**
     * Searches contact list for specified @a pattern.
     */
    search_result search_all (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::alias_field) const
    {
        search_result sr;

        _pcl->for_each([& sr, & pattern, sf] (contact::contact const & c) {
            contact::contact const * pc = & c;

            if (sf.value & search_flags::alias_field) {
                search_all_helper(c.alias, pattern, sf.value & search_flags::ignore_case
                    , [& sr, pc] (pfs::unicode::match_item const & m) {
                        sr.m.emplace_back(match_item{pc->contact_id, search_flags::alias_field, m});

                        if (sr.sp.empty() || sr.sp.back().contact_id != pc->contact_id) {
                            sr.sp.emplace_back(match_spec{pc->contact_id, sr.m.size() - 1, 1});
                        } else {
                            sr.sp.back().count++;
                        }
                    });
            }

            if (sf.value & search_flags::desc_field) {
                search_all_helper(c.description, pattern, sf.value & search_flags::ignore_case
                    , [& sr, pc] (pfs::unicode::match_item const & m) {
                        sr.m.emplace_back(match_item{pc->contact_id, search_flags::desc_field, m});

                        if (sr.sp.empty() || sr.sp.back().contact_id != pc->contact_id) {
                            sr.sp.emplace_back(match_spec{pc->contact_id, sr.m.size() - 1, 1});
                        } else {
                            sr.sp.back().count++;
                        }
                    });
            }
        });

        return sr;
    }
};

template <typename Conversation>
class conversation_searcher
{
    using utf8_input_iterator = pfs::unicode::utf8_input_iterator<std::string::const_iterator>;

private:
    Conversation const * _pcv {nullptr};

public:
    struct match_item
    {
        message::id message_id;
        int content_index; // index of content in message_credentials::contents
        pfs::unicode::match_item m;
    };

    struct search_result
    {
        std::vector<match_item> m;
    };

public:
    conversation_searcher (Conversation const & cv)
        : _pcv(& cv)
    {}

public:
    /**
     * Searches all contents of all conversion messages for specified @a pattern.
     */
    search_result search_all (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_result sr;

        _pcv->for_each([& sr, & pattern, sf] (message::message_credentials const & mc) {
            if (mc.contents) {
                for (int i = 0, count = mc.contents->count(); i < count; i++) {
                    auto cc = mc.contents->at(i);

                    bool is_text_content = cc.mime == message::mime_enum::text__plain
                        || cc.mime == message::mime_enum::text__html;

                    bool content_search_requested = is_text_content
                        ? (sf.value & search_flags::text_content)
                        : (sf.value & search_flags::attachment_name);

                    auto first = utf8_input_iterator::begin(cc.text.begin(), cc.text.end());
                    auto s_first = utf8_input_iterator::begin(pattern.begin(), pattern.end());

                    if (content_search_requested) {
                        if (cc.mime == message::mime_enum::text__html) {
                            pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
                                , sf.value & search_flags::ignore_case, '<', '>'
                                , [& sr, & mc, i] (pfs::unicode::match_item const & m) {
                                    sr.m.emplace_back(match_item{mc.message_id, i, m});
                                });
                        } else {
                            pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
                                , sf.value & search_flags::ignore_case
                                , [& sr, & mc, i] (pfs::unicode::match_item const & m) {
                                    sr.m.emplace_back(match_item{mc.message_id, i, m});
                                });
                        }
                    }
                }
            }
        });

        return sr;
    }
};

template <typename MessageStore, typename ContactList>
class messages_searcher
{
    using utf8_input_iterator = pfs::unicode::utf8_input_iterator<std::string::const_iterator>;
    using conversation_type = typename MessageStore::conversation_type;
    using conversation_searcher_type = conversation_searcher<conversation_type>;

private:
    MessageStore const * _pms {nullptr};
    ContactList const *  _pcl {nullptr};

public:
    messages_searcher (MessageStore const & ms, ContactList const & cl)
        : _pms(& ms)
        , _pcl(& cl)
    {}

public:
    struct match_item
    {
        contact::id contact_id;
        message::id message_id;
        int content_index; // index of content in message_credentials::contents
        pfs::unicode::match_item m;
    };

    struct search_result
    {
        std::vector<match_item> m;
    };

public:
    /**
     * Searches all messages for specified @a pattern.
     * Only the first message content that matches the @a pattern matters.
     */
    search_result search_all (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_result sr;

        _pcl->for_each([this, & sr, & pattern, sf] (chat::contact::contact const & c) {
            auto cv = _pms->conversation(c.contact_id);

            if (!cv)
                return;

            cv.for_each([& sr, & c, & pattern, sf] (message::message_credentials const & mc) {
                if (mc.contents) {
                    for (int i = 0, count = mc.contents->count(); i < count; i++) {
                        auto cc = mc.contents->at(i);

                        bool is_text_content = cc.mime == message::mime_enum::text__plain
                            || cc.mime == message::mime_enum::text__html;

                        bool content_search_requested = is_text_content
                            ? (sf.value & search_flags::text_content)
                            : (sf.value & search_flags::attachment_name);

                        auto first = utf8_input_iterator::begin(cc.text.begin(), cc.text.end());
                        auto s_first = utf8_input_iterator::begin(pattern.begin(), pattern.end());

                        if (content_search_requested) {
                            if (cc.mime == message::mime_enum::text__html) {
                                auto m = pfs::unicode::search_first(first, first.end(), s_first, s_first.end()
                                    , sf.value & search_flags::ignore_case, '<', '>');

                                if (m.cp_first >= 0) {
                                    sr.m.emplace_back(match_item{c.contact_id, mc.message_id, i, m});
                                    break;
                                };
                            } else {
                                auto m = pfs::unicode::search_first(first, first.end(), s_first, s_first.end()
                                    , sf.value & search_flags::ignore_case);

                                if (m.cp_first >= 0) {
                                    sr.m.emplace_back(match_item{c.contact_id, mc.message_id, i, m});
                                    break;
                                }
                            }
                        }
                    }
                }
            });
        });

        return sr;
    }
};

} // namespace chat
