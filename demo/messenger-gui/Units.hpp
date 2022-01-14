////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <QObject>
#include <QScreen>

QT_BEGIN_NAMESPACE
class QGuiApplication;
QT_END_NAMESPACE

namespace ui {
namespace qt {

class Units: public QObject
{
    Q_OBJECT

public:
    enum FormFactor
    {
          Unknown
        , Phone
        , Phablet
        , Tablet
        , Desktop
        , TV
    };
    Q_ENUM(FormFactor);

private:
    Q_PROPERTY(double scaleFactor
        READ scaleFactor
        WRITE setScaleFactor
        NOTIFY scaleFactorChanged)

    Q_SIGNAL void scaleFactorChanged (double factor);

private:
    FormFactor _formFactor;
    double     _scaleFactor;
    double     _pixelDensityPerInch;

private:
    double scaleFactor () const noexcept
    {
        return _scaleFactor;
    }

public:
    Units (QObject * parent = nullptr);

    /**
     * Changes display form factor.
     *
     * @brief After this call scale factor resets to it's defaults according to
     *        specfied form factor.
     */
    Q_INVOKABLE void setFormFactor (FormFactor formFactor);

    /**
     * Sets pixel density in pixels-per-inch.
     */
    void setPixelDensityPerInch (double dpi);

    /**
     * Sets scale factor
     */
    void setScaleFactor (double factor);

    /**
     * Converts independent pixels to physical pixels.
     */
    Q_INVOKABLE int dp (int x) const noexcept;
};

class PixelDensityObserver
{
private:
    QScreen * _screen = nullptr;
    Units *   _units  = nullptr;
    QMetaObject::Connection _physicalDpiChangedConnection;

private:
    /**
     * Calculate diagonal in inches.
     */
    static double calculateDiagonal (double pixelDensity, int width, int height);

    /**
     * Converts diagonal size into display form factor.
     */
    static Units::FormFactor formFactorByDiagonal (double diagonal);

    void onScreenChanged (QScreen * screen);
    void updateUnits (QScreen * screen);

public:
    PixelDensityObserver (QGuiApplication * app, Units * units);
};

}} // namespace ui::qt

// Q_DECLARE_METATYPE(ui::qt::Units)
