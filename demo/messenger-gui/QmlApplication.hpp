////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `chat-lib`.
//
// Changelog:
//      2022.01.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Units.hpp"
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <memory>

namespace ui {
namespace qt {

class QmlApplication: public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QGuiApplication>       _app;
    std::unique_ptr<QQmlApplicationEngine> _engine;
    std::unique_ptr<PixelDensityObserver>  _pixelDensityObserver;
    Units _units;

public:
    QmlApplication (int & argc, char ** argv, QObject * parent = nullptr);

    operator bool () const noexcept;

    QQmlContext & rootContext ();
    int exec ();
};

}} // namespace ui::qt
