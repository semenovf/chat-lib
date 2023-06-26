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
#include "message.hpp"
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
    using utf8_iterator = pfs::unicode::utf8_iterator<std::string::const_iterator>;

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
    contacts_searcher (ContactList const & cl) : _pcl(& cl) {}

private:
    static void search_all_helper (std::string const & s, std::string const & pattern
        , bool ignore_case, std::function<void(pfs::unicode::match_item const &)> && f)
    {
        auto first = utf8_iterator::begin(s.begin(), s.end());
        auto s_first = utf8_iterator::begin(pattern.begin(), pattern.end());

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

class message_searcher
{
    using utf8_iterator = pfs::unicode::utf8_iterator<std::string::const_iterator>;

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

private:
    contact::id _contact_id;
    message::message_credentials const * _pmc {nullptr};

public:
    message_searcher (contact::id contact_id, message::message_credentials const & mc)
        : _contact_id(contact_id), _pmc(& mc)
    {}

private:
    void search_helper (search_result & sr, std::string const & pattern
        , bool first_match_only
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        if (_pmc->contents) {
            for (int i = 0, count = _pmc->contents->count(); i < count; i++) {
                auto cc = _pmc->contents->at(i);

                bool is_text_content = cc.mime == mime_enum::text__plain
                    || cc.mime == mime_enum::text__html;

                bool content_search_requested = is_text_content
                    ? (sf.value & search_flags::text_content)
                    : (sf.value & search_flags::attachment_name);

                auto first = utf8_iterator::begin(cc.text.begin(), cc.text.end());
                auto s_first = utf8_iterator::begin(pattern.begin(), pattern.end());

                if (content_search_requested) {
                    if (cc.mime == mime_enum::text__html) {
                        if (first_match_only) {
                            auto m = pfs::unicode::search_first(first, first.end(), s_first, s_first.end()
                                , sf.value & search_flags::ignore_case, '<', '>');

                            if (m.cp_first >= 0) {
                                sr.m.emplace_back(match_item{_contact_id, _pmc->message_id, i, m});
                                break;
                            };
                        } else {
                            pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
                                , sf.value & search_flags::ignore_case, '<', '>'
                                , [this, & sr, i] (pfs::unicode::match_item const & m) {
                                    sr.m.emplace_back(match_item{_contact_id, _pmc->message_id, i, m});
                                });
                        }
                    } else {
                        if (first_match_only) {
                            auto m = pfs::unicode::search_first(first, first.end(), s_first, s_first.end()
                                , sf.value & search_flags::ignore_case);

                            if (m.cp_first >= 0) {
                                sr.m.emplace_back(match_item{_contact_id, _pmc->message_id, i, m});
                                break;
                            }
                        } else {
                            pfs::unicode::search_all(first, first.end(), s_first, s_first.end()
                                , sf.value & search_flags::ignore_case
                                , [this, & sr, i] (pfs::unicode::match_item const & m) {
                                    sr.m.emplace_back(match_item{_contact_id, _pmc->message_id, i, m});
                                });
                        }
                    }
                }
            }
        }
    }

public:
    void search_all (search_result & sr, std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_helper(sr, pattern, false, sf);
    }

    search_result search_all (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_result sr;
        search_helper(sr, pattern, false, sf);
        return sr;
    }

    void search_first (search_result & sr, std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_helper(sr, pattern, true, sf);
    }

    search_result search_first (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_result sr;
        search_helper(sr, pattern, true, sf);
        return sr;
    }
};

template <typename Conversation>
class conversation_searcher
{
public:
    using match_item    = message_searcher::match_item;
    using search_result = message_searcher::search_result;

private:
    contact::id _contact_id;
    Conversation const * _pcv {nullptr};

public:
    conversation_searcher (contact::id contact_id, Conversation const & cv)
        : _contact_id(contact_id)
        , _pcv(& cv)
    {}

public:
    /**
     * Searches all contents of all conversion messages for specified @a pattern.
     */
    search_result search_all (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_result sr;

        _pcv->for_each([this, & sr, & pattern, sf] (message::message_credentials const & mc) {
            message_searcher{_contact_id, mc}.search_all(sr, pattern, sf);
        });

        return sr;
    }

    search_result search_first (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_result sr;

        _pcv->for_each([this, & sr, & pattern, sf] (message::message_credentials const & mc) {
            message_searcher{_contact_id, mc}.search_first(sr, pattern, sf);
        });

        return sr;
    }
};

template <typename MessageStore, typename ContactList>
class message_store_searcher
{
    using conversation_type = typename MessageStore::conversation_type;
    using conversation_searcher_type = conversation_searcher<conversation_type>;

public:
    using match_item    = message_searcher::match_item;
    using search_result = message_searcher::search_result;

private:
    MessageStore const * _pms {nullptr};
    ContactList const *  _pcl {nullptr};

public:
    message_store_searcher (MessageStore const & ms, ContactList const & cl)
        : _pms(& ms)
        , _pcl(& cl)
    {}

public:
    /**
     * Searches all messages for specified @a pattern.
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
                message_searcher{c.contact_id, mc}.search_all(sr, pattern, sf);
            });
        });

        return sr;
    }

    /**
     * Searches all messages for specified @a pattern.
     * Only the first message content that matches the @a pattern matters.
     */
    search_result search_first (std::string const & pattern
        , search_flags sf = search_flags::ignore_case | search_flags::text_content) const
    {
        search_result sr;

        _pcl->for_each([this, & sr, & pattern, sf] (chat::contact::contact const & c) {
            auto cv = _pms->conversation(c.contact_id);

            if (!cv)
                return;

            cv.for_each([& sr, & c, & pattern, sf] (message::message_credentials const & mc) {
                message_searcher{c.contact_id, mc}.search_first(sr, pattern, sf);
            });
        });

        return sr;
    }
};

} // namespace chat
