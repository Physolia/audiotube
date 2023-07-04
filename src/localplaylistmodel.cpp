// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "localplaylistmodel.h"

#include "library.h"

LocalPlaylistModel::LocalPlaylistModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(this, &LocalPlaylistModel::playlistIdChanged,
            this, &LocalPlaylistModel::refreshModel);
    connect(this, &LocalPlaylistModel::playlistIdChanged,
            this, &LocalPlaylistModel::refreshModel);
}

int LocalPlaylistModel::rowCount(const QModelIndex &index) const
{
    return index.isValid() ? 0 : m_entries.size();
}

QHash<int, QByteArray> LocalPlaylistModel::roleNames() const
{
    return {
        { Roles::VideoId, "videoId" },
        { Roles::Title, "title" },
        { Roles::Artists, "artists"},
        { Roles::ArtistsDisplayString, "artistsDisplayString"}

    };
}

QVariant LocalPlaylistModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Roles::Title:
        return m_entries[index.row()].title;
    case Roles::VideoId:
        return m_entries[index.row()].videoId;
    case Roles::Artists:
        return QVariant::fromValue(std::vector<meta::Artist> {
            {
                m_entries.at(index.row()).artists.toStdString(),
                {}
            }
        });
    case Roles::ArtistsDisplayString:
        return m_entries.at(index.row()).artists;
    }

    Q_UNREACHABLE();
}

QString LocalPlaylistModel::playlistId() const
{
    return m_playlistId;
}

void LocalPlaylistModel::setPlaylistId(const QString &playlistId)
{
    m_playlistId = playlistId;
    Q_EMIT playlistIdChanged();
}

void LocalPlaylistModel::refreshModel()
{
    auto future = Library::instance()
            .database()
            .getResults<PlaylistEntry>(
                "select video_id, title, artist, album from "
                "playlist_entries natural join songs where playlist_id = ?", m_playlistId);

    QCoro::connect(std::move(future), this, [this](auto entries) {
        beginResetModel();
        m_entries = entries;
        endResetModel();
    });
}

const std::vector<PlaylistEntry> &LocalPlaylistModel::entries() const
{
    return m_entries;
}

void LocalPlaylistModel::removeSong(QString videoId, qint64 playlistId)
{
    QCoro::connect(Library::instance().database().execute("delete from playlist_entries where playlist_id = ? and video_id = ?", playlistId, videoId), this, &LocalPlaylistModel::refreshModel);
}

bool LocalPlaylistModel::exportPlaylist(QUrl const& filePath)
{
    if(!filePath.isLocalFile()) {
        return false;
    }
    QFile file(filePath.path(), this);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    for(size_t i = 0; i<m_entries.size(); ++i) {
        out << "https://music.youtube.com/watch?v=" << m_entries[i].videoId << '\n';
    }

    return true;
}
