import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import skywalker

ListView {
    required property list<InviteCode> codes
    required property bool failedToLoad

    signal closed
    signal authorClicked(string did)

    id: inviteCodeList
    width: parent.width
    clip: true
    model: codes

    header: Rectangle {
        width: parent.width
        height: guiSettings.headerHeight
        z: guiSettings.headerZLevel
        color: guiSettings.headerColor

        RowLayout {
            width: parent.width
            height: guiSettings.headerHeight

            SvgButton {
                id: backButton
                iconColor: "white"
                Material.background: "transparent"
                svg: svgOutline.arrowBack
                onClicked: inviteCodeList.closed()
            }
            Text {
                Layout.alignment: Qt.AlignVCenter
                font.bold: true
                font.pointSize: guiSettings.scaledFont(10/8)
                color: "white"
                text: qsTr("Invite Codes")
            }
        }
    }

    delegate: Rectangle {
        required property var modelData // class InviteCode

        id: codeEntry
        width: inviteCodeList.width
        height: codeRow.height + usedByRow.height

        RowLayout {
            id: codeRow
            width: parent.width

            Column {
                Layout.fillWidth: true

                Text {
                    width: parent.width
                    leftPadding: 10
                    topPadding: 10
                    font.strikeout: !modelData.available || modelData.disabled
                    color: modelData.disabled ? "grey" : guiSettings.textColor
                    text: modelData.code
                }
                Text {
                    width: parent.width
                    leftPadding: 10
                    font.pointSize: guiSettings.scaledFont(7/8)
                    color: "grey"
                    text: modelData.createdAt.toLocaleDateString(Qt.locale(), Locale.LongFormat)
                }
            }

            SvgButton {
                id: copyButton
                svg: svgOutline.copy
                iconColor: guiSettings.textColor
                Material.background: "transparent"
                visible: modelData.available && !modelData.disabled
                onClicked: modelData.copyToClipboard()
            }
        }
        GridLayout {
            id: usedByRow
            anchors.top: codeRow.bottom
            width: parent.width
            columns: 2
            rowSpacing: 2

            // Avatar
            Rectangle {
                id: avatar
                Layout.rowSpan: 3
                width: 65
                height: avatarImg.height + 20
                Layout.fillHeight: true
                visible: modelData.used

                Avatar {
                    id: avatarImg
                    x: parent.x + 8
                    y: parent.y + 10
                    width: parent.width - 12
                    height: width
                    avatarUrl: modelData.usedBy.avatarUrl
                    onClicked: authorClicked(modelData.usedByDid)
                }
            }

            Text {
                id: nameText
                topPadding: 5
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: modelData.usedBy.name
                visible: modelData.used
            }

            Text {
                id: handleText
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pointSize: guiSettings.scaledFont(7/8)
                color: guiSettings.handleColor
                text: `@${modelData.usedBy.handle}`
                visible: modelData.used
            }

            Text {
                id: dateText
                Layout.fillWidth: true
                Layout.fillHeight: true
                font.pointSize: guiSettings.scaledFont(7/8)
                color: "grey"
                text: modelData.usedAt.toLocaleDateString(Qt.locale(), Locale.LongFormat)
                visible: modelData.used
            }

            Rectangle {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "lightgrey"
            }
        }
        MouseArea {
            z: -1
            anchors.fill: parent
            enabled: modelData.used
            onClicked: authorClicked(modelData.usedByDid)
        }
    }

    Text {
        leftPadding: 10
        y: parent.headerItem ? parent.headerItem.height + 10 : 0
        width: parent.width
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        text: qsTr("Could not retrieve invite codes. To retrieve invite codes you need to sign in with your real password, not an app password.")
        visible: failedToLoad
    }

    Text {
        leftPadding: 10
        y: parent.headerItem ? parent.headerItem.height + 10 : 0
        width: parent.width
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        text: qsTr("You do not yet have any invide codes.")
        visible: !failedToLoad && inviteCodeList.count === 0
    }

    GuiSettings {
        id: guiSettings
    }
}
