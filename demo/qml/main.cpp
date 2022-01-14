////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2021.12.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

int main (int argc, char * argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    QUrl url{QStringLiteral("qrc:/main.qml")};

    //qmlRegisterSingletonType(QUrl("qrc:/Units.qml"), "MyUnits", 1, 0, "Units" );
//     engine.addImportPath(":/");
    engine.load(url);

    Q_ASSERT(!engine.rootObjects().isEmpty());

    return app.exec();
}
