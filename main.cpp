// Copyright (C) 2023 Michel de Boer
// License: GPLv3
#include "atproto_image_provider.h"
#include "font_downloader.h"
#include "shared_image_provider.h"
#include "skywalker.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFont>
#include <QString>

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{time HH:mm:ss.zzz} %{type} %{function}'%{line} %{message}");

    QGuiApplication app(argc, argv);
    app.setOrganizationName(Skywalker::Skywalker::APP_NAME);
    app.setApplicationName(Skywalker::Skywalker::APP_NAME);

    Skywalker::FontDownloader::initAppFonts();

    QQmlApplicationEngine engine;
    auto* providerId = Skywalker::SharedImageProvider::SHARED_IMAGE;
    engine.addImageProvider(providerId, Skywalker::SharedImageProvider::getProvider(providerId));
    providerId = Skywalker::ATProtoImageProvider::DRAFT_IMAGE;
    engine.addImageProvider(providerId, Skywalker::ATProtoImageProvider::getProvider(providerId));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    using namespace Qt::Literals::StringLiterals;
    const auto url = "qrc:/skywalker/Main.qml"_L1;
    engine.load(url);

    return app.exec();
}
