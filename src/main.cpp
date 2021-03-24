// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <KLocalizedContext>

#include "searchmodel.h"
#include "albummodel.h"
#include "videoinfoextractor.h"
#include "artistmodel.h"
#include "userplaylistmodel.h"
#include "errorhandler.h"
#include "playlistmodel.h"

constexpr auto URI = "org.kde.ytmusic";

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("audiotube"));

    QQmlApplicationEngine engine;

    qmlRegisterType<SearchModel>(URI, 1, 0, "SearchModel");
    qmlRegisterType<AlbumModel>(URI, 1, 0, "AlbumModel");
    qmlRegisterType<VideoInfoExtractor>(URI, 1, 0, "VideoInfoExtractor");
    qmlRegisterType<ArtistModel>(URI, 1, 0, "ArtistModel");
    qmlRegisterType<PlaylistModel>(URI, 1, 0, "PlaylistModel");
    qmlRegisterSingletonType<UserPlaylistModel>(URI, 1, 0, "UserPlaylistModel", [](QQmlEngine *, QJSEngine *) {
        return new UserPlaylistModel();
    });
    qmlRegisterSingletonType<ErrorHandler>(URI, 1, 0, "ErrorHandler", [](QQmlEngine *, QJSEngine *) {
        return new ErrorHandler();
    });

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:///main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
