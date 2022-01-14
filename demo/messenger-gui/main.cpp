////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "ContactModel.hpp"
#include "ConversationModel.hpp"
#include "Messenger.hpp"
#include "MessengerProxyForQml.hpp"
#include "QmlApplication.hpp"
#include "pfs/memory.hpp"
#include <QCoreApplication>

int main (int argc, char * argv[])
{
    auto consoleApplication = false;
    auto messenger = std::make_shared<Messenger>();

    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "-no-gui")) {
            consoleApplication = true;
            break;
        }
    }

    if (consoleApplication) {
        QCoreApplication app(argc, argv);
        return app.exec();
    }

    auto messengerProxyForQml = pfs::make_unique<ui::qt::MessengerProxyForQml>(messenger);

    //
    // Register types before ui::qt::QmlApplication
    //
    qmlRegisterUncreatableType<ui::qt::MessengerProxyForQml>("chat.lib", 1, 0
        , "MessengerProxyForQml", "MessengerProxyForQml is uncreatable");

    qRegisterMetaType<ui::qt::ContactModel*>("ContactModel*");
    qRegisterMetaType<ui::qt::ConversationModel*>("ConversationModel*");

    ui::qt::QmlApplication app(argc, argv);

    if (!app)
        return EXIT_FAILURE;

    auto & rootContext = app.rootContext();
    rootContext.setContextProperty("messengerProxy", & *messengerProxyForQml);

    return app.exec();
}
