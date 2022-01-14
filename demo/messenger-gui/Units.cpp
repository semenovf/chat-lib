////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "Units.hpp"
#include <QGuiApplication>
#include <QDebug>
#include <memory>
#include <cmath>

// [Pixel density](https://en.wikipedia.org/wiki/Pixel_density)
//
// NOTE:
// QScreen.physicalDotsPerInch() - pixel density in pixels-per-inch
// Screen.pixelDensity           - pixel density pixels-per-millimeter.
//
namespace ui {
namespace qt {

namespace {

// Baseline pixel densities in pixels-per-inch
constexpr double BASELINE_DENSITY[] = {
       96.0 // Unknown
    , 160.0 // Phone   (according Android spec)
    , 160.0 // Phablet (as Phone)
    , 160.0 // Tablet  (as Phone)
    ,  96.0 // Desktop (according Microsoft spec)
    ,  96.0 // TV      (as Desktop)
};

constexpr double MILLIMETERS_PER_INCH  = 25.4;
constexpr double INCHES_PER_MILLIMETER = 0.039370;

} // namespace

Units::Units (QObject * parent)
    : QObject(parent)
    , _formFactor(FormFactor::Unknown)
    , _scaleFactor(1.0)
    , _pixelDensityPerInch(BASELINE_DENSITY[_formFactor])
{}

void Units::setFormFactor (FormFactor formFactor)
{
    _formFactor = formFactor;

    switch (_formFactor) {
        case FormFactor::Desktop:
        case FormFactor::TV:
            setScaleFactor(1.4);
            break;
        case FormFactor::Phone:
        case FormFactor::Phablet:
        case FormFactor::Tablet:
        default:
            setScaleFactor(1.0);
            break;
    }
}

void Units::setScaleFactor (double factor)
{
    if (_scaleFactor != factor) {
        _scaleFactor = factor;
        qDebug().noquote().nospace() << "Scale factor: " << _scaleFactor;
        Q_EMIT(scaleFactorChanged(_scaleFactor));
    }
}

void Units::setPixelDensityPerInch (double dpi)
{
    _pixelDensityPerInch = dpi;
}

int Units::dp (int x) const noexcept
{
    auto baseLine = BASELINE_DENSITY[_formFactor];
    auto result = std::round(x * (_pixelDensityPerInch / baseLine) /* * _scaleFactor*/);
    return static_cast<int>(result);
}

PixelDensityObserver::PixelDensityObserver (QGuiApplication * app, Units * units)
    : _units(units)
{
    Q_ASSERT(app);
    Q_ASSERT(units);

    updateUnits(app->primaryScreen());

    auto conn = std::make_shared<QMetaObject::Connection>();

    *conn = QObject::connect(app, & QGuiApplication::applicationStateChanged
        , [this, app, conn] (Qt::ApplicationState state) {

        if (state == Qt::ApplicationActive) {
            // This signal need only for the first time
            QObject::disconnect(*conn);
            onScreenChanged(app->primaryScreen());
        }
    });

    QObject::connect(app, & QGuiApplication::primaryScreenChanged
        , [this] (QScreen * screen) {

        qDebug() << "Primary screen changed";
        onScreenChanged(screen);
    });

    QObject::connect(app, & QGuiApplication::screenAdded, [] (QScreen *) {
        // TODO Does it useful ?
        qDebug() << "Screen added";
    });

    QObject::connect(app, & QGuiApplication::screenRemoved, [] (QScreen *) {
        // TODO Does it useful ?
        qDebug() << "Screen removed";
    });
}

void PixelDensityObserver::onScreenChanged (QScreen * screen)
{
    if (screen != _screen) {
        auto oldScreen = _screen;
        _screen = screen;

        if (oldScreen)
            QObject::disconnect(_physicalDpiChangedConnection);

        if (_screen) {
            _physicalDpiChangedConnection = QObject::connect(_screen
                , & QScreen::physicalDotsPerInchChanged
                , [this] (double /*dpi*/) {
                updateUnits(_screen);
            });
        }

        // May be this is a first initialization with valid screen
        if (!oldScreen)
            updateUnits(_screen);
    }
}

inline double PixelDensityObserver::calculateDiagonal (double pixelDensity
    , int width
    , int height)
{
    auto diagonal = std::sqrt(width * width + height * height) / pixelDensity;
    return diagonal;
}

Units::FormFactor PixelDensityObserver::formFactorByDiagonal (double diagonal)
{
    if (diagonal < 6.5)
        return Units::Phone;
    if (diagonal < 7.0)
        return Units::Phablet;
    if (diagonal < 10.1)
        return Units::Tablet;
    if (diagonal < 42.0)
        return Units::Desktop;
    return Units::TV;
}

void PixelDensityObserver::updateUnits (QScreen * screen)
{
    Q_ASSERT(screen);

    auto dpi = screen->physicalDotsPerInch();
    auto rect = screen->geometry();
    auto diagonal   = calculateDiagonal(dpi, rect.width(), rect.height());
    auto formFactor = formFactorByDiagonal(diagonal);

    _units->setFormFactor(formFactor);
    _units->setPixelDensityPerInch(dpi);

    qDebug().noquote().nospace() << "Units updated: dp(1)=" << _units->dp(1);
}

}} // namespace ui::qt
