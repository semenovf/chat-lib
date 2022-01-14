////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "QmlApplication.hpp"
#include "pfs/fmt.hpp"
#include "pfs/memory.hpp"
#include <QDebug>
#include <QFileInfo>

#define CRITICAL(s) qCritical().noquote().nospace() << ((s).c_str())
#define DEBUG(s)    qDebug().noquote().nospace() << ((s).c_str())

namespace ui {
namespace qt {

namespace {
constexpr char const * INIT_QML_FILE  = ":/Init.qml";
constexpr char const * UNITS_QML_FILE = ":/Units.qml";
constexpr char const * APP_QML_FILE   = ":/QmlApplication.qml";
} // namespace

QmlApplication::QmlApplication (int & argc, char ** argv, QObject * parent)
    : QObject(parent)
{
    auto success = true;

    for (auto const & rc: {
          INIT_QML_FILE
        , UNITS_QML_FILE
        , APP_QML_FILE}) {

        if (!QFileInfo::exists(rc)) {
            CRITICAL(fmt::format("Resource not found: {}", rc));
            success = false;
        }
    }

    if (!success)
        return;

    _app = pfs::make_unique<QGuiApplication>(argc, argv);
    _engine = pfs::make_unique<QQmlApplicationEngine>();

    QQmlContext * rootContext = _engine->rootContext();
    Q_ASSERT(rootContext);

    //rootContext->setContextProperty("unitsBackend", QVariant::fromValue<Units>(_units));
    rootContext->setContextProperty("unitsBackend", & _units);

    _pixelDensityObserver = pfs::make_unique<PixelDensityObserver>(& *_app, & _units);

    // FIXME
    QObject::connect(& *_app, & QGuiApplication::applicationStateChanged
        , [] (Qt::ApplicationState state) {

        std::string stateStr {"Unknown application state"};

        switch (state) {
            // On Linux this state not raised.
            case Qt::ApplicationSuspended:
                stateStr = "SUSPENDED";
                break;

            // On Linux this state not raised.
            case Qt::ApplicationHidden:
                stateStr = "HIDDEN";
                break;

            // On Linux
            // This state raised when:
            //     - application windows moving (by mouse);
            //     - Alt-Tab pressed before application window will be hidden;
            //     - after switching to another application.
            //     - after minimized window.
            case Qt::ApplicationInactive:
                stateStr = "INACTIVE";
                break;

            // On Linux
            // This state raised when
            //     - application window appears on front of the screen at startup;
            //     - after restoring window size after minimization.
            case Qt::ApplicationActive: {
                stateStr = "ACTIVE";
                break;
            }
        }

        DEBUG(fmt::format("Application state: {}", stateStr));
    });

    qmlRegisterSingletonType(QUrl(QString{"qrc"} + UNITS_QML_FILE)
        , "units.ui", 1, 0, "Units");

    // FIXME
    // QObject::connect(& *_engine, & QQmlApplicationEngine::quit, [this] {
    //    this->quit();
    // });
}

QmlApplication::operator bool () const noexcept
{
    return (_app && _engine);
}

QQmlContext & QmlApplication::rootContext ()
{
    QQmlContext * rootContext = _engine->rootContext();
    Q_ASSERT(rootContext);
    return *rootContext;
}

int QmlApplication::exec ()
{
    Q_ASSERT(_app);

    _engine->load(QUrl(QString{"qrc"} + INIT_QML_FILE));

    if (_engine->rootObjects().isEmpty()) {
        CRITICAL(fmt::format("no QML files loaded"));
        return EXIT_FAILURE;
    }

    return _app->exec();
}

}} // namespace ui::qt
