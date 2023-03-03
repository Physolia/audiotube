// SPDX-FileCopyrightText: 2022 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.ytmusic 1.0
import org.kde.kirigami 2.19 as Kirigami
import QtQuick.Layouts 1.15


Item {
    ShareMenu {
        id: shareMenu
    }
    function openForSong(videoId, songTitle, artists, artistsDisplayString) {
        shareMenu.url = "https://music.youtube.com/watch?v=" + videoId
        shareMenu.inputTitle= songTitle

        menu.videoId = videoId
        drawer.videoId = videoId

        menu.songTitle = songTitle
        drawer.songTitle = songTitle

        menu.artists = artists
        drawer.artists = artists

        menu.artistsDisplayString = artistsDisplayString
        drawer.artistsDisplayString = artistsDisplayString

        if (Kirigami.Settings.isMobile) {
            drawer.interactive = true
            drawer.open()
         } else {
            menu.popup()
        }
    }
    Controls.Drawer {
        edge: Qt.BottomEdge
        height:contents.implicitHeight+20
        width: applicationWindow().width
        interactive: false
        background: Kirigami.ShadowedRectangle{
            corners.topRightRadius: 10
            corners.topLeftRadius: 10
            shadow.size: 20
            shadow.color: Qt.rgba(0, 0, 0, 0.5)
            color: Kirigami.Theme.backgroundColor

        }
        onClosed: drawer.interactive = false

        id: drawer
        property string videoId
        property string songTitle
        property var artists
        property string artistsDisplayString
        ColumnLayout {
            id: contents
            anchors.fill: parent
            ThumbnailSource {
                id: thumbnailSource
                videoId: drawer.videoId
            }

            Rectangle {
                Layout.margins: 5
                radius:50
                Layout.alignment: Qt.AlignHCenter
                color: Kirigami.Theme.textColor
                opacity: 0.7
                width: 40
                height: 4

            }
            RowLayout {
                spacing: 10
                Kirigami.ShadowedRectangle {
                    Layout.margins: 10
                    color: Kirigami.Theme.backgroundColor
                    width: 60
                    height: width
                    radius: 5
                    shadow.size: 15
                    shadow.xOffset: 5
                    shadow.yOffset: 5
                    shadow.color: Qt.rgba(0, 0, 0, 0.2)
                    RoundedImage {
                        source: thumbnailSource.cachedPath
                        height: width
                        width: parent.width
                        radius: parent.radius
                    }
                }
                ColumnLayout {
                    Controls.Label{
                        text: drawer.songTitle
                        elide: Text.ElideRight
                        Layout.fillWidth: true


                    }
                    Controls.Label{
                        text: drawer.artistsDisplayString
                        color: Kirigami.Theme.disabledTextColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true

                    }
                }
            }
            Kirigami.Separator {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10


            }
            Kirigami.BasicListItem{

                label: i18n("Play Next")
                icon: "go-next"
                onClicked: {
                    UserPlaylistModel.playNext(drawer.videoId, drawer.songTitle, drawer.artists)
                    drawer.close()
                }
            }
            Kirigami.BasicListItem{

                label: i18n("Add to queue")
                icon: "media-playlist-append"
                onClicked: {
                    UserPlaylistModel.append(drawer.videoId, drawer.songTitle, drawer.artists)
                    drawer.close()
                }
            }
            Kirigami.BasicListItem{
                readonly property QtObject favouriteWatcher: Library.favouriteWatcher(drawer.videoId)

                label: favouriteWatcher ? (favouriteWatcher.isFavourite ? i18n("Remove Favourite"): i18n("Add Favourite")): ""
                icon: favouriteWatcher ? (favouriteWatcher.isFavourite ? "starred-symbolic" : "non-starred-symbolic") : ""
                onClicked: {
                    if (favouriteWatcher) {
                        if (favouriteWatcher.isFavourite) {
                            Library.removeFavourite(drawer.videoId)
                        } else {
                            Library.addFavourite(drawer.videoId, drawer.songTitle, drawer.artistsDisplayString, "")
                        }
                    }
                    drawer.close()
                }
            }

            Kirigami.BasicListItem{
                readonly property QtObject wasPlayedWatcher: Library.wasPlayedWatcher(drawer.videoId)
                
                label: i18n("Remove from History")
                icon: "list-remove"
                onClicked: {
                    Library.removePlaybackHistoryItem(drawer.videoId)
                    drawer.close()
                }
                visible: wasPlayedWatcher ? wasPlayedWatcher.wasPlayed : false
                enabled: wasPlayedWatcher ? wasPlayedWatcher.wasPlayed : false
            }

            Kirigami.BasicListItem{
                readonly property QtObject wasPlayedWatcher: Library.wasPlayedWatcher(drawer.videoId)

                label: i18n("Share Song")
                icon: "emblem-shared-symbolic"
                onClicked: {
                    shareMenu.open()
                    drawer.close()
                }
            }
            Item{
                Layout.fillHeight: true
            }

        }
    }

    Controls.Menu {
        property string videoId
        property string songTitle
        property var artists
        property string artistsDisplayString
        id: menu

        Controls.MenuItem {
            text: i18n("Play Next")
            icon.name: "go-next"
            onTriggered: UserPlaylistModel.playNext(menu.videoId, menu.songTitle, menu.artists)
        }

        Controls.MenuItem {
            text: i18n("Add to queue")
            icon.name: "media-playlist-append"
            onTriggered: UserPlaylistModel.append(menu.videoId, menu.songTitle, menu.artists)
        }


        Controls.MenuSeparator{}

        Controls.MenuItem {
            readonly property QtObject favouriteWatcher: Library.favouriteWatcher(menu.videoId)
            text: favouriteWatcher ? (favouriteWatcher.isFavourite ? i18n("Remove Favourite"): i18n("Add Favourite")): ""
            icon.name: favouriteWatcher ? (favouriteWatcher.isFavourite ? "starred-symbolic" : "non-starred-symbolic") : ""
            onTriggered: {
                if (favouriteWatcher) {
                    if (favouriteWatcher.isFavourite) {
                        Library.removeFavourite(menu.videoId)
                    } else {
                        Library.addFavourite(menu.videoId, menu.songTitle, menu.artistsDisplayString, "")
                    }
                }
            }
        }
        Controls.MenuItem{
            readonly property QtObject wasPlayedWatcher: Library.wasPlayedWatcher(menu.videoId)
            text: i18n("Remove from History")
            icon.name: "list-remove"
            onTriggered: {
                Library.removePlaybackHistoryItem(menu.videoId)
            }
            enabled: wasPlayedWatcher ? wasPlayedWatcher.wasPlayed : false
            visible: wasPlayedWatcher ? wasPlayedWatcher.wasPlayed : false
        }
        Controls.MenuSeparator{}

        Controls.MenuItem {
            text: i18n("Share Song")
            icon.name: "emblem-shared-symbolic"
            onTriggered: shareMenu.open()
        }
    }
}
