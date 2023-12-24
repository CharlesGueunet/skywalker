import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import skywalker

Rectangle {
    property int margin: 10
    required property int viewWidth
    required property generatorview feed
    required property profile feedCreator
    required property bool feedSaved
    required property bool feedPinned
    required property bool endOfFeed
    property int maxTextLines: 1000

    signal feedClicked(generatorview feed)

    id: generatorView
    width: grid.width
    height: grid.height
    color: "transparent"

    GridLayout {
        id: grid
        rowSpacing: 0
        columns: 3
        width: viewWidth

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            height: 5
            color: "transparent"
        }

        FeedAvatar {
            Layout.leftMargin: generatorView.margin
            Layout.rightMargin: generatorView.margin
            x: 8
            y: 5
            width: guiSettings.threadBarWidth * 5
            avatarUrl: feed.avatar

            onClicked: feedClicked(feed)
        }

        Column {
            spacing: 5
            Layout.fillWidth: true
            Layout.rightMargin: generatorView.margin

            Text {
                width: parent.width
                elide: Text.ElideRight
                font.bold: true
                color: guiSettings.textColor
                text: feed.displayName
            }

            Text {
                width: parent.width
                elide: Text.ElideRight
                color: guiSettings.textColor
                text: feedCreator.name
            }

            Text {
                width: parent.width
                elide: Text.ElideRight
                font.pointSize: guiSettings.scaledFont(7/8)
                color: guiSettings.handleColor
                text: "@" + feedCreator.handle
            }
        }

        Rectangle {
            width: 80
            Layout.fillHeight: true

            SvgImage {
                id: favoIcon
                anchors.right: addIcon.left
                width: 40
                height: width
                color: feedPinned ? guiSettings.favoriteColor : guiSettings.statsColor
                svg: feedPinned ? svgFilled.star : svgOutline.star
            }

            SvgButton {
                id: addIcon
                anchors.right: parent.right
                width: 40
                height: width
                iconColor: guiSettings.buttonTextColor
                Material.background: guiSettings.buttonColor
                svg: feedSaved ? svgOutline.remove : svgOutline.add
            }
        }

        Text {
            topPadding: 5
            bottomPadding: 10
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.leftMargin: generatorView.margin
            Layout.rightMargin: generatorView.margin
            wrapMode: Text.Wrap
            maximumLineCount: maxTextLines
            elide: Text.ElideRight
            textFormat: Text.RichText
            color: guiSettings.textColor
            text: feed.description
        }

        Rectangle {
            height: likeIcon.height
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.leftMargin: generatorView.margin
            Layout.rightMargin: generatorView.margin
            color: "transparent"

            StatIcon {
                id: likeIcon
                iconColor: feed.viewer.like ? guiSettings.likeColor : guiSettings.statsColor
                svg: feed.viewer.like ? svgFilled.like : svgOutline.like
                statistic: feed.likeCount
            }
        }

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            height: 5
            color: "transparent"
        }

        Rectangle {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: guiSettings.separatorColor
        }

        // End of feed indication
        Text {
            Layout.columnSpan: 3
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            topPadding: generatorView.margin
            elide: Text.ElideRight
            color: guiSettings.textColor
            text: qsTr("End of feed")
            font.italic: true
            visible: endOfFeed
        }
    }

    MouseArea {
        z: -2 // Let other mouse areas on top
        anchors.fill: parent
        onClicked: {
            console.debug("FEED CLICKED:", feed.displayName)
            generatorView.feedClicked(feed)
        }
    }

    GuiSettings {
        id: guiSettings
    }
}