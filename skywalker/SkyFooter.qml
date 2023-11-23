import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import skywalker

Rectangle {
    required property var timeline
    required property var skywalker
    property bool homeActive: false
    property bool notificationsActive: false
    property bool searchActive: false
    property int unreadNotifications: 0

    signal homeClicked()
    signal notificationsClicked()
    signal searchClicked()

    width: parent.width
    height: guiSettings.footerHeight
    z: guiSettings.footerZLevel
    color: guiSettings.footerColor

    Rectangle {
        id: separatorLine
        width: parent.width
        height: 1
        color: guiSettings.separatorColor
    }

    RowLayout {
        anchors.top: separatorLine.bottom
        width: parent.width
        height: parent.height - separatorLine.height

        Rectangle {
            height: parent.height
            Layout.fillWidth: true
            color: guiSettings.backgroundColor

            SvgImage {
                id: homeButton
                y: height + 5
                width: height
                height: parent.height - 10
                anchors.horizontalCenter: parent.horizontalCenter
                color: guiSettings.textColor
                svg: homeActive ? svgFilled.home : svgOutline.home

                Rectangle {
                    x: parent.width - 17
                    y: -parent.y + 6
                    width: Math.max(unreadCountText.width + 10, height)
                    height: 20
                    radius: 8
                    color: guiSettings.badgeColor
                    border.color: guiSettings.badgeBorderColor
                    border.width: 2
                    visible: timeline.unreadPosts > 0

                    Text {
                        id: unreadCountText
                        anchors.centerIn: parent
                        font.bold: true
                        font.pointSize: guiSettings.scaledFont(6/8)
                        color: guiSettings.badgeTextColor
                        text: timeline.unreadPosts
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: homeClicked()
            }
        }

        Rectangle {
            height: parent.height
            Layout.fillWidth: true
            color: guiSettings.backgroundColor

            SvgImage {
                id: searchButton
                y: height + 5
                width: height
                height: parent.height - 10
                anchors.horizontalCenter: parent.horizontalCenter
                color: guiSettings.textColor
                svg: searchActive ? svgFilled.search : svgOutline.search
            }

            MouseArea {
                anchors.fill: parent
                onClicked: searchClicked()
            }
        }

        Rectangle {
            height: parent.height
            Layout.fillWidth: true
            color: guiSettings.backgroundColor

            SvgImage {
                id: notificationsButton
                y: height + 5
                width: height
                height: parent.height - 10
                anchors.horizontalCenter: parent.horizontalCenter
                color: guiSettings.textColor
                svg: notificationsActive ? svgFilled.notifications : svgOutline.notifications

                Rectangle {
                    x: parent.width - 17
                    y: -parent.y + 6
                    width: Math.max(unreadNotificationsText.width + 10, height)
                    height: 20
                    radius: 8
                    color: guiSettings.badgeColor
                    border.color: guiSettings.badgeBorderColor
                    border.width: 2
                    visible: skywalker.unreadNotificationCount > 0

                    Text {
                        id: unreadNotificationsText
                        anchors.centerIn: parent
                        font.bold: true
                        font.pointSize: guiSettings.scaledFont(6/8)
                        color: guiSettings.badgeTextColor
                        text: skywalker.unreadNotificationCount
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: notificationsClicked()
            }
        }
    }

    PostButton {
        x: parent.width - width - 10
        y: -height - 10
    }

    GuiSettings {
        id: guiSettings
    }
}