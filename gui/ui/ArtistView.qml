/*
 * Copyright (C) 2016, 2017
 *      Jean-Luc Barriere <jlbarriere68@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.9
import QtQuick 2.9
import QtQuick.Controls 2.2
import NosonApp 1.0
import "../components"
import "../components/Delegates"
import "../components/Flickables"
import "../components/ViewButton"

MusicPage {
    id: artistViewPage
    objectName: "artistViewPage"
    visible: false
    pageFlickable: artistAlbumView

    property var containerItem: null

    property string artistSearch: ""
    property string artist: ""
    property var covers: []

    property bool isFavorite: false

    BlurredBackground {
        id: blurredBackground
        height: parent.height
    }

    MusicGridView {
        id: artistAlbumView
        itemWidth: units.gu(15)
        heightOffset: units.gu(7)

        header: MusicHeader {
            id: blurredHeader
            isFavorite: artistViewPage.isFavorite
            rightColumn: Column {
                spacing: units.gu(2)
                ShuffleButton {
                    model: songArtistModel
                    width: blurredHeader.width > units.gu(60) ? units.gu(23.5) : (blurredHeader.width - units.gu(13)) / 2
                }
                QueueAllButton {
                    containerItem: artistViewPage.containerItem
                    width: blurredHeader.width > units.gu(60) ? units.gu(23.5) : (blurredHeader.width - units.gu(13)) / 2
                }
                PlayAllButton {
                    containerItem: artistViewPage.containerItem
                    width: blurredHeader.width > units.gu(60) ? units.gu(23.5) : (blurredHeader.width - units.gu(13)) / 2
                }
            }
            height: contentHeight
            noCover: "qrc:/images/none.png"
            coverSources: artistViewPage.covers
            titleColumn: Column {
                spacing: units.gu(1)

                Label {
                    id: artistLabel
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    color: styleMusic.common.music
                    elide: Text.ElideRight
                    font.pointSize: units.fs("x-large")
                    maximumLineCount: 1
                    text: artist != "" ? artist : i18n.tr("Unknown Artist")
                    wrapMode: Text.NoWrap
                }

                Row {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        id: albumCount
                        color: styleMusic.common.subtitle
                        elide: Text.ElideRight
                        font.pointSize: units.fs("small")
                        maximumLineCount: 1
                        text: qsTr("%n album(s)", "", albumsModel.count)
                    }

                    Label {
                        id: separator
                        color: styleMusic.common.subtitle
                        elide: Text.ElideRight
                        font.pointSize: units.fs("small")
                        maximumLineCount: 1
                        text: " , "
                        visible: songArtistModel.count > 0
                    }

                    Label {
                        id: songCount
                        color: styleMusic.common.subtitle
                        elide: Text.ElideRight
                        font.pointSize: units.fs("small")
                        maximumLineCount: 1
                        text: qsTr("%n song(s)", "", songArtistModel.totalCount)
                        visible: songArtistModel.count > 0
                    }
                }

                Item {
                    id: spacer
                    width: parent.width
                    height: units.gu(1)
                }
            }

            onFirstSourceChanged: {
                blurredBackground.art = firstSource
            }
        }

        model: AlbumsModel {
            id: albumsModel
            onDataUpdated: albumsModel.asyncLoad()
            onLoaded: albumsModel.resetModel()
            Component.onCompleted: {
                init(Sonos, artistSearch, false)
                albumsModel.asyncLoad()
            }
        }
        delegate: Card {
            id: albumCard
            coverSources: [
                {art: makeCoverSource(model.art, model.artist, model.title)},
                {art: makeCoverSource(undefined, model.artist, model.title)}
            ]
            primaryText: model.title !== "" ? model.title : qsTr("Unknown Album")
            secondaryTextVisible: false

            onClicked: {
                stackView.push("qrc:/ui/SongsView.qml",
                                   {
                                       "containerItem": makeContainerItem(model),
                                       "songSearch": model.id,
                                       "album": model.title,
                                       "artist": model.artist,
                                       "covers": [{art: (albumCard.imageSource != "" ? albumCard.imageSource : model.art)}],
                                       "isAlbum": true,
                                       "genre": "",
                                       "pageTitle": qsTr("Album"),
                                       "line1": model.artist !== undefined && model.artist !== "" ? model.artist : qsTr("Unknown Artist"),
                                       "line2": model.title !== undefined && model.title !== "" ? model.title : qsTr("Unknown Album")
                                   })
            }

            // check favorite on data loaded
            Connections {
                target: AllFavoritesModel
                onCountChanged: {
                    albumCard.isFavorite = (AllFavoritesModel.findFavorite(model.payload).length > 0)
                }
            }

            Component.onCompleted: {
                isFavorite = (AllFavoritesModel.findFavorite(model.payload).length > 0)
            }

        }
    }

    // Page actions
    optionsMenuVisible: true
    optionsMenuContentItems: [
        MenuItem {
            text: artistViewPage.isFavorite ?  qsTr("Remove from favorites") : qsTr("Add to favorites")
            font.pointSize: units.fs("medium")
            onTriggered: {
                if (!artistViewPage.isFavorite) {
                    if (addItemToFavorites(containerItem, pageTitle, "" /*artistAlbumView.headerItem.firstSource*/))
                        artistViewPage.isFavorite = true
                } else {
                    if (removeFromFavorites(containerItem.payload))
                        artistViewPage.isFavorite = false
                }
            }
        }
    ]

    // check favorite on data loaded
    Connections {
        target: AllFavoritesModel
        onCountChanged: {
            isFavorite = (AllFavoritesModel.findFavorite(containerItem.payload).length > 0)
        }
    }

    // Query total count of artist's songs
    TracksModel {
        id: songArtistModel
        onDataUpdated: songArtistModel.asyncLoad()
        onLoaded: songArtistModel.resetModel()
        Component.onCompleted: {
            songArtistModel.init(Sonos, artistSearch + "/", false)
            songArtistModel.asyncLoad()
        }
    }

    Component.onCompleted: {
        isFavorite = (AllFavoritesModel.findFavorite(containerItem.payload).length > 0)
    }
}
