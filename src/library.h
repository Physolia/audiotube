// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#pragma once

#include <QObject>
#include <QQuickAsyncImageProvider>
#include <QNetworkAccessManager>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include <memory>

#include "asyncytmusic.h"
#include "asyncdatabase.h"

class FavouriteWatcher;
class WasPlayedWatcher;

struct Song {
    using ColumnTypes = std::tuple<QString, QString, QString, QString>;

    static Song fromSql(ColumnTypes tuple) {
        auto [videoId, title, artist, album] = tuple;
        return Song {videoId, title, artist, album};
    }

    QString videoId;
    QString title;
    QString artist;
    QString album;
};

class FavouritesModel : public QAbstractListModel {
    Q_OBJECT

    enum Roles {
        VideoId = Qt::UserRole + 1,
        Title,
        Artists,
        ArtistsDisplayString
    };

public:
    FavouritesModel(QFuture<std::vector<Song>> &&songs, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    std::vector<Song> getFavouriteSongs() const;

private:
    std::vector<Song> m_favouriteSongs;
};

struct PlayedSong {
    using ColumnTypes = std::tuple<QString, int, QString, QString, QString>;

    static PlayedSong fromSql(ColumnTypes tuple) {
        auto [videoId, plays, title, artist, album] = tuple;
        return PlayedSong {videoId, title, artist, album, plays};
    }

    QString videoId;
    QString title;
    QString artist;
    QString album;
    int plays;
};

class PlaybackHistoryModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        VideoId = Qt::UserRole + 1,
        Title,
        Artists,
        ArtistsDisplayString,
        Plays
    };
    Q_ENUM(Roles);

    PlaybackHistoryModel(QFuture<std::vector<PlayedSong>> &&songs, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    std::vector<PlayedSong> getPlayedSong() const;

private:
    std::vector<PlayedSong> m_playedSongs;
};

class SearchHistoryModel : public QAbstractListModel {
    Q_OBJECT

public:
    SearchHistoryModel(QFuture<std::vector<SingleValue<QString>>> &&historyFuture, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    void removeSearch(const QString &search);
    QVariant data(const QModelIndex &index, int role) const override;
    void addSearch(QString const& search);

private:
    size_t getRow(QString const &search) const;
    std::vector<SingleValue<QString>> m_history;
};

class Library : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *favourites READ favourites NOTIFY favouritesChanged)
    Q_PROPERTY(QAbstractListModel *searches READ searches CONSTANT)
    Q_PROPERTY(QAbstractListModel *playbackHistory READ playbackHistory NOTIFY playbackHistoryChanged)
    Q_PROPERTY(QAbstractListModel *mostPlayed READ mostPlayed NOTIFY playbackHistoryChanged)

public:
    explicit Library(QObject *parent = nullptr);
    ~Library();

    static Library &instance();

    FavouritesModel *favourites();
    Q_SIGNAL void favouritesChanged();
    Q_SIGNAL void changeFavourites();
    Q_INVOKABLE void addFavourite(const QString &videoId, const QString &title, const QString &artist, const QString &album);
    Q_INVOKABLE void removeFavourite(const QString &videoId);
    Q_INVOKABLE FavouriteWatcher *favouriteWatcher(const QString &videoId);

    SearchHistoryModel *searches();
    Q_SIGNAL void searchesChanged();
    Q_INVOKABLE void addSearch(const QString &text);
    Q_INVOKABLE void addTemporarySearch(const QString &text);
    Q_INVOKABLE void removeSearch(const QString &text);
    Q_INVOKABLE void removeTemporarySearch(const QString &text);

    PlaybackHistoryModel *playbackHistory();
    Q_SIGNAL void playbackHistoryChanged();
    Q_SIGNAL void changePlaybackHistory();
    Q_INVOKABLE void addPlaybackHistoryItem(const QString &videoId, const QString &title, const QString &artist, const QString &album);
    Q_INVOKABLE void removePlaybackHistoryItem(const QString &videoId);
    Q_INVOKABLE WasPlayedWatcher *wasPlayedWatcher(const QString &videoId);

    PlaybackHistoryModel *mostPlayed();

    QNetworkAccessManager &nam();
    ThreadedDatabase &database() {
        return *m_database;
    }

private:
    QFuture<void> addSong(const QString &videoId, const QString &title, const QString &artist, const QString &album);

    QNetworkAccessManager m_networkImageCacher;
    std::unique_ptr<ThreadedDatabase> m_database;
    SearchHistoryModel *m_searches;
    FavouritesModel *m_favourites;
    PlaybackHistoryModel *m_mostPlayed;
    PlaybackHistoryModel *m_playbackHistory;
};


class ThumbnailSource : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString videoId READ videoId WRITE setVideoId NOTIFY videoIdChanged)
    Q_PROPERTY(QUrl cachedPath READ cachedPath NOTIFY cachedPathChanged)

    QString videoId() const {
        return m_videoId;
    }
    void setVideoId(const QString &id);
    Q_SIGNAL void videoIdChanged();

    QUrl cachedPath() const {
        return m_cachedPath;
    }
    void setCachedPath(const QUrl &path) {
        m_cachedPath = path;
        cachedPathChanged();
    }
    Q_SIGNAL void cachedPathChanged();

private:
    QString m_videoId;
    QUrl m_cachedPath;
};

class FavouriteWatcher : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isFavourite READ isFavourite NOTIFY isFavouriteChanged)

public:
    FavouriteWatcher(Library *library, const QString &videoId);

    bool isFavourite() const;
    Q_SIGNAL void isFavouriteChanged();

    Q_SIGNAL void videoIdChanged();

private:
    QString m_videoId;
    Library *m_library;
    bool m_isFavourite = false;
};

class WasPlayedWatcher : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(bool wasPlayed READ wasPlayed NOTIFY wasPlayedChanged)
    
public:
    WasPlayedWatcher(Library *Library, const QString &VideoId);
    
    bool wasPlayed() const;
    
    Q_SIGNAL void wasPlayedChanged();

private:
    bool m_wasPlayed = false;
    QString m_videoId;
    Library *m_library;
    Q_SLOT void update(std::optional<SingleValue<bool>> result);
    Q_SLOT void query();
};
