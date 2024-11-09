import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import skywalker

Rectangle {
    required property convoview convo
    property var skywalker: root.getSkywalker()
    property basicprofile firstMember: convo.members.length > 0 ? convo.members[0].basicProfile : skywalker.getUserProfile()
    readonly property int margin: 10

    signal back

    id: headerRect
    width: parent.width
    height: GuiSettings.headerHeight
    z: GuiSettings.headerZLevel
    color: GuiSettings.headerColor

    Accessible.role: Accessible.Pane

    RowLayout {
        id: convoRow
        width: parent.width
        height: parent.height
        spacing: 5

        SvgButton {
            id: backButton
            iconColor: GuiSettings.headerTextColor
            Material.background: "transparent"
            svg: SvgOutline.arrowBack
            accessibleName: qsTr("go back")
            onClicked: headerRect.back()
        }

        Avatar {
            id: avatar
            Layout.alignment: Qt.AlignVCenter
            width: parent.height - 10
            height: width
            author: firstMember
            onClicked: skywalker.getDetailedProfile(firstMember.did)
        }

        Column {
            id: convoColumn
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 5
            Layout.rightMargin: margin
            spacing: 0

            SkyCleanedTextLine {
                width: parent.width
                Layout.fillHeight: true
                elide: Text.ElideRight
                font.bold: true
                color: GuiSettings.headerTextColor
                plainText: convo.memberNames
            }

            AccessibleText {
                width: parent.width
                Layout.preferredHeight: visible ? implicitHeight : 0
                color: GuiSettings.handleColor
                font.pointSize: GuiSettings.scaledFont(7/8)
                text: `@${firstMember.handle}`
                visible: convo.members.length <= 1
            }

            Row {
                width: parent.width
                spacing: 3

                Repeater {
                    model: convo.members.slice(1)

                    Avatar {
                        required property chatbasicprofile modelData

                        width: 25
                        height: width
                        author: modelData.basicProfile
                    }
                }

                visible: convo.members.length > 1
            }
        }
    }

}
