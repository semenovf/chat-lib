////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`
//
// Changelog:
//      2021.12.27 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ControllerBuilder.hpp"
#include "pfs/fmt.hpp"
#include "pfs/memory.hpp"

std::unique_ptr<ControllerBuilder::type> ControllerBuilder::operator () ()
{
    type controller;
    controller.failure.connect([] (std::string const & err) {
        fmt::print(stderr, err);
        fmt::print(stderr, "\n");
    });

    return pfs::make_unique<type>(std::move(controller));
}
