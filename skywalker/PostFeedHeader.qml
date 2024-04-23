import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import skywalker

Rectangle {
    required property var skywalker
    required property string feedName
    property bool isList: false
    property string feedAvatar
    property bool showAsHome: false
    property bool isHomeFeed: false

    signal closed
    signal feedAvatarClicked

    id: header
    width: parent.width
    height: guiSettings.headerHeight
    z: guiSettings.headerZLevel
    color: guiSettings.headerColor

    Accessible.role: Accessible.Pane

    RowLayout {
        id: headerRow
        width: parent.width
        height: guiSettings.headerHeight

        SvgButton {
            id: backButton
            iconColor: guiSettings.headerTextColor
            Material.background: "transparent"
            svg: svgOutline.arrowBack
            accessibleName: qsTr("go back")
            visible: !showAsHome

            onClicked: header.closed()
        }
        FeedAvatar {
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            height: parent.height - 10
            width: height
            avatarUrl: header.feedAvatar
            unknownSvg: isList ? svgFilled.list : svgFilled.feed
            visible: showAsHome && !isHomeFeed

            onClicked: header.feedAvatarClicked()

            Accessible.role: Accessible.Button
            Accessible.name: header.feedName
            Accessible.onPressAction: clicked()
        }
        SkyCleanedText {
            id: headerTexts
            Layout.fillWidth: true //!showAsHome
            Layout.alignment: Qt.AlignVCenter
            leftPadding: header.feedAvatar ? 0 : 10
            rightPadding: showAsHome ? expandFeedsButton.width : 0
            elide: Text.ElideRight
            font.bold: true
            font.pointSize: guiSettings.scaledFont(10/8)
            color: guiSettings.headerTextColor
            plainText: header.feedName

            Accessible.role: Accessible.ButtonDropDown
            Accessible.name: qsTr(`${header.feedName}, press to select other feed`)
            Accessible.description: Accessible.name
            Accessible.onPressAction: expandFeeds()

            ExpandFeedsButton {
                id: expandFeedsButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                skywalker: header.skywalker
                Layout.alignment: Qt.AlignLeft
                visible: showAsHome
            }

            MouseArea {
                anchors.fill: parent
                onClicked: parent.expandFeeds()
            }

            function expandFeeds() {
                if (expandFeedsButton.visible)
                    expandFeedsButton.onClicked()
            }
        }

        FeedAvatar {
            Layout.rightMargin: 10
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            height: parent.height - 10
            width: height
            avatarUrl: header.feedAvatar
            visible: !showAsHome && !isHomeFeed

            onClicked: header.feedAvatarClicked()

            Accessible.role: Accessible.Button
            Accessible.name: header.feedName
            Accessible.description: Accessible.name
            Accessible.onPressAction: header.feedAvatarClicked()
        }
        Item {
            Layout.rightMargin: 10
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            height: parent.height - 10
            width: height
            visible: showAsHome
            Accessible.role: Accessible.Pane

            Avatar {
                id: avatar
                width: parent.width
                height: parent.height
                avatarUrl: skywalker.avatarUrl
                onClicked: root.showSettingsDrawer()
                onPressAndHold: root.showSwitchUserDrawer()

                Accessible.role: Accessible.ButtonMenu
                Accessible.name: qsTr("Skywalker menu")
                Accessible.description: Accessible.name
                Accessible.onPressAction: clicked()
            }
        }
    }

    GuiSettings {
        id: guiSettings
    }
}
