// SPDX-FileCopyrightText: 2023 Théophile Gilgien <theophile@gilgien.net>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "playlistimporter.h"
#include "playlistutils.h"

#include "library.h"
#include <qfuture.h>
#include <QFile>
#include <qglobal.h>
#include <qsqldatabase.h>
#include <threadeddatabase.h>

#include <KLocalizedString>

PlaylistImporter::PlaylistImporter(QObject* parent)
    :QObject(parent)
{}


void PlaylistImporter::importPlaylist(const QString &url)
{
    const QString croppedURL = this->cropURL(url).toString(), title = i18n("Unknown"), description = i18n("No description");
    QCoro::connect(Library::instance().database().execute("insert into playlists (title, description) values (?, ?)", title, description), &Library::instance(), [this, croppedURL]() {
        QCoro::connect(Library::instance().database().getResults<SingleValue<qint64>>("select * from playlists"), &Library::instance(), [this, croppedURL](const auto& playlists) {
            const quint64 playlistId = playlists.back().value;
            Q_EMIT Library::instance().playlistsChanged();

            QCoro::connect(YTMusicThread::instance()->fetchPlaylist(croppedURL), this, [this, playlistId](const auto& playlist) {
                this->renamePlaylist(playlistId, QString::fromStdString(playlist.title), QString::fromStdString(playlist.author.name));

                for (const auto& track : playlist.tracks) {
                    if (track.is_available && track.video_id) {
                        this->addPlaylistEntry(playlistId, track);
                    }
                }

                Q_EMIT Library::instance().playlistsChanged();
                Q_EMIT importFinished();
            });
        });
    });
}

void PlaylistImporter::addPlaylistEntry(qint64 playlistId, const QString &videoId, const QString &title, const QString &artist, const QString &album)
{
    QCoro::connect(Library::instance().addSong(videoId, title, artist, album), this, [=, this] {
        QCoro::connect(Library::instance().database().execute("insert into playlist_entries (playlist_id, video_id) values (?, ?)", playlistId, videoId), this, [=, this] {
            Q_EMIT playlistEntriesChanged(playlistId);
        });
    });
}

void PlaylistImporter::addPlaylistEntry(qint64 playlistId, const playlist::Track &track)
{
    const QString videoId = track.video_id.value().c_str();
    const QString title   = (!track.title.empty()) ? QString::fromStdString(track.title) : i18n("No title");
    const QString artists = PlaylistUtils::artistsToString(track.artists);
    const QString album   = (track.album ) ? QString::fromStdString(track.album->name) : i18n("No album");
    this->addPlaylistEntry(playlistId, videoId, title, artists, album);
}

void PlaylistImporter::addPlaylistEntry(qint64 playlistId, const song::Song& song)
{
    const QString videoId = song.video_id.c_str();
    const QString title = song.title.c_str();
    const QString artist = song.author.c_str();
    this->addPlaylistEntry(playlistId, videoId, title, artist, "");
}


void PlaylistImporter::importPlaylistFromFile(const QUrl& filePath)
{
    if(!filePath.isLocalFile()) {
        Q_EMIT importFailed(filePath.path());
        return;
    }
    const QString title = filePath.fileName();
    const QString description = i18n("No description");
    QString path(filePath.path());
    QCoro::connect(Library::instance().database().execute("insert into playlists (title, description) values (?, ?)", title, description), &Library::instance(), [this, path]() {
        QCoro::connect(Library::instance().database().getResults<SingleValue<qint64>>("select * from playlists"), &Library::instance(), [this, path](const auto& playlists) {
            const quint64 playlistId = playlists.back().value;
            QFile file(path, this);
            if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                Q_EMIT importFailed(path);
                return;
            }
            QTextStream in(&file);
            Q_EMIT Library::instance().playlistsChanged();
            while(!in.atEnd()) {
                in.skipWhiteSpace();
                QString url = in.readLine();
                if(url[0] != '#') {
                    int x = url.indexOf("v=");
                    QString videoId = url.right(url.size() - x - 2);
                    videoId.truncate(11);
                    QCoro::connect(YTMusicThread::instance()->fetchSong(videoId), this, [this, playlistId](const auto& song) {
                        if(song) {
                            this->addPlaylistEntry(playlistId, song.value());
                            Q_EMIT Library::instance().playlistsChanged();
                        }
                    });;
                }
            }
        });
    });
}


void PlaylistImporter::renamePlaylist(qint64 playlistId, const QString &name, const QString &description)
{
    QCoro::connect(Library::instance().database().execute("update playlists set title = ? , description = ? where playlist_id = ?", name, description, playlistId), this, &PlaylistImporter::refreshModel);
}


QStringView PlaylistImporter::cropURL(QStringView srcUrl)
{
    // Find entry point
    constexpr auto urlFragment = QStringView(u"?list=");
    qsizetype urlPos = srcUrl.indexOf(urlFragment);
    if (urlPos != -1) {
        urlPos += urlFragment.size();
    } else {
        urlPos = 0;
    }
    auto mid = srcUrl.mid(urlPos);

    // Find exit point
    urlPos = std::min(mid.indexOf(u"?"), mid.indexOf(u"&"));
    return mid.mid(0, urlPos);
}
