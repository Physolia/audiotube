import QtQuick 2.1
import org.kde.kirigami 2.14 as Kirigami
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.3

import org.kde.ytmusic 1.0

Kirigami.ScrollablePage {
    property alias browseId: albumModel.browseId
    title: albumModel.title

    actions {
        main: Kirigami.Action {
            icon.name: "media-playback-start"
            text: i18n("Play")
            onTriggered: {
                applicationWindow().playPlaylist(albumModel.playlistId)
            }
        }
    }

    ListView {
        header: Kirigami.ItemViewHeader {
            backgroundImage.source: albumModel.thumbnailUrl
            title: albumModel.title
        }

        model: AlbumModel {
            id: albumModel
        }
        delegate: Kirigami.BasicListItem {
            required property string title
            required property string videoId

            icon: "emblem-music-symbolic"
            text: title
            onClicked: play(videoId)
        }

        Controls.BusyIndicator {
            anchors.centerIn: parent
            visible: albumModel.loading
        }
    }
}
