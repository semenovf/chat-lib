////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "MessageStoreBuilder.hpp"
#include "pfs/memory.hpp"

std::unique_ptr<MessageStoreBuilder::type> MessageStoreBuilder::operator () ()
{
    namespace fs = pfs::filesystem;

    auto on_failure = [] (std::string const & errstr) {
        fmt::print(stderr, "ERROR: {}\n", errstr);
    };

    auto messageStorePath = fs::temp_directory_path() / "messenger";

    if (!fs::exists(messageStorePath)) {
        std::error_code ec;

        if (!fs::create_directory(messageStorePath, ec)) {
            on_failure(fmt::format("Create directory failure: {}: {}"
                , pfs::filesystem::utf8_encode(messageStorePath)
                , ec.message()));
            return nullptr;
        }
    }

    messageStorePath /= "messages.db";

    auto dbh = chat::persistent_storage::sqlite3::make_handle(messageStorePath
        , true, on_failure);

    if (!dbh)
        return nullptr;

    auto messageStore = pfs::make_unique<MessageStoreBuilder::type>(dbh, on_failure);

    if (!*messageStore)
        return nullptr;

    messageStore->wipe();

    return messageStore;
}
