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
#include "pfs/filesystem.hpp"
#include "pfs/memory.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <memory>

namespace {

auto ON_FAILURE = [] (std::string const & errstr) {
    fmt::print(stderr, "ERROR: {}\n", errstr);
};

} // namespace

QCoreApplication * createApplication (int & argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
        if (!qstrcmp(argv[i], "-no-gui"))
            return new QCoreApplication(argc, argv);
    return new QGuiApplication(argc, argv);
}

bool runGuiApplication (SharedMessenger messenger, std::unique_ptr<QCoreApplication> && app)
{
    auto contactModel = std::make_shared<ContactModel>(messenger->contact_list_shared());
    auto messengerProxyForQml = pfs::make_unique<MessengerProxyForQml>(contactModel);

    std::unique_ptr<QQmlApplicationEngine> engine;

    qmlRegisterUncreatableType<MessengerProxyForQml>("pfs.chat", 1, 0
        , "MessengerProxyForQml", "MessengerProxyForQml is uncreatable");

    qmlRegisterUncreatableType<ContactModel>("pfs.chat", 1, 0
        , "ContactModel", "ContactModel is uncreatable");

    qmlRegisterUncreatableType<ConversationModel>("pfs.chat", 1, 0
        , "ConversationModel", "ConversationModel is uncreatable");

    engine.reset(new QQmlApplicationEngine);

    QQmlContext * rootContext = engine->rootContext();
    rootContext->setContextProperty("messengerProxyForQml", & *messengerProxyForQml);

    engine->load(QUrl(QStringLiteral("qrc:/Main.qml")));

    return app->exec();
}

int runConsoleApplication (SharedMessenger, std::unique_ptr<QCoreApplication> && app)
{
    return app->exec();
}

std::unique_ptr<Messenger::controller_type> createController ()
{
    auto controller = pfs::make_unique<Messenger::controller_type>();
    return controller;
}

int main (int argc, char * argv[])
{
    auto messenger = std::make_shared<Messenger>();

    std::unique_ptr<QCoreApplication> app(createApplication(argc, argv));

    if (qobject_cast<QGuiApplication *>(& *app)) {
        return runGuiApplication(messenger, std::move(app));
    } else {
        return runConsoleApplication(messenger, std::move(app));
    }

    return EXIT_SUCCESS;
}
