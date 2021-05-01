// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "artistmodel.h"

#include <asyncytmusic.h>

ArtistModel::ArtistModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(this, &ArtistModel::channelIdChanged, this, [=] {
        if (m_channelId.isEmpty()) {
            return;
        }

        setLoading(true);

        YTMusicThread::instance()->fetchArtist(m_channelId);
    });
    connect(&YTMusicThread::instance().get(), &AsyncYTMusic::fetchArtistFinished, this, [=](const artist::Artist &artist) {
        setLoading(false);

        beginResetModel();
        m_artist = artist;
        std::sort(m_artist.thumbnails.begin(), m_artist.thumbnails.end());
        m_view = std::optional(MultiIterableView(m_artist.albums->results,
                                                 m_artist.singles->results,
                                                 m_artist.songs->results,
                                                 m_artist.videos->results));
        endResetModel();


        Q_EMIT titleChanged();
        Q_EMIT thumbnailUrlChanged();
    });
    connect(&YTMusicThread::instance().get(), &AsyncYTMusic::errorOccurred, this, [=] {
        setLoading(false);
    });
}

int ArtistModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_view ? int(m_view->size()) : 0;
}

QVariant ArtistModel::data(const QModelIndex &index, int role) const
{
    if (!m_view) {
        return {};
    }

    switch (role) {
    case Title:
        return QString::fromStdString(std::visit([&](auto&& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr(std::is_same_v<T, artist::Artist::Album>) {
                return item.title;
            } else if constexpr(std::is_same_v<T, artist::Artist::Single>) {
                return item.title;
            } else if constexpr(std::is_same_v<T, artist::Artist::Song>) {
                return item.title;
            } else if constexpr(std::is_same_v<T, artist::Artist::Video>) {
                return item.title;
            }

            Q_UNREACHABLE();
        }, m_view.value()[index.row()]));
    case Type:
        return std::visit([&](auto&& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr(std::is_same_v<T, artist::Artist::Album>) {
                return Type::Album;
            } else if constexpr(std::is_same_v<T, artist::Artist::Single>) {
                return Type::Single;
            } else if constexpr(std::is_same_v<T, artist::Artist::Song>) {
                return Type::Song;
            } else if constexpr(std::is_same_v<T, artist::Artist::Video>) {
                return Type::Video;
            }

            Q_UNREACHABLE();
        }, m_view.value()[index.row()]);
    case Artists:
        return QVariant::fromValue(std::vector<meta::Artist> {
            {
                m_artist.name,
                m_artist.channel_id
            }
        });
    case VideoId:
        return std::visit([&](auto&& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr(std::is_same_v<T, artist::Artist::Album>) {
                return QVariant();
            } else if constexpr(std::is_same_v<T, artist::Artist::Single>) {
                return QVariant();
            } else if constexpr(std::is_same_v<T, artist::Artist::Song>) {
                return QVariant(QString::fromStdString(item.video_id));
            } else if constexpr(std::is_same_v<T, artist::Artist::Video>) {
                return QVariant(QString::fromStdString(item.video_id));
            }

            Q_UNREACHABLE();
        }, m_view.value()[index.row()]);
    }

    Q_UNREACHABLE();

    return {};
}

QHash<int, QByteArray> ArtistModel::roleNames() const
{
    return {
        {Title, "title"},
        {Type, "type"},
        {Artists, "artists"},
        {VideoId, "videoId"}
    };
}

QString ArtistModel::channelId() const
{
    return m_channelId;
}

void ArtistModel::setChannelId(const QString &channelId)
{
    m_channelId = channelId;
    Q_EMIT channelIdChanged();
}

QString ArtistModel::title() const
{
    return QString::fromStdString(m_artist.name);
}

QUrl ArtistModel::thumbnailUrl() const
{
    if (m_artist.thumbnails.empty()) {
        return QUrl();
    }

    return QUrl(QString::fromStdString(m_artist.thumbnails.back().url));
}

bool ArtistModel::loading() const
{
    return m_loading;
}

void ArtistModel::setLoading(bool loading)
{
    m_loading = loading;
    Q_EMIT loadingChanged();
}

void ArtistModel::triggerItem(int row)
{
    if (m_view) {
        std::visit([&](auto&& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr(std::is_same_v<T, artist::Artist::Album>) {
                Q_EMIT openAlbum(QString::fromStdString(item.browse_id));
            } else if constexpr(std::is_same_v<T, artist::Artist::Single>) {
                Q_EMIT openAlbum(QString::fromStdString(item.browse_id));
            } else if constexpr(std::is_same_v<T, artist::Artist::Song>) {
                Q_EMIT openSong(QString::fromStdString(item.video_id));
            } else if constexpr(std::is_same_v<T, artist::Artist::Video>) {
                Q_EMIT openVideo(QString::fromStdString(item.video_id));
            } else {
                Q_UNREACHABLE();
            }
        }, m_view.value()[row]);
    }
}
