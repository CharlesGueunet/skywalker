import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import skywalker

ListView {
    required property int modelId
    required property int postEntryIndex
    signal closed

    id: view
    spacing: 0
    model: skywalker.getPostThreadModel(modelId)
    ScrollIndicator.vertical: ScrollIndicator {}

    header: Rectangle {
        width: parent.width
        height: guiSettings.headerHeight
        z: guiSettings.headerZLevel
        color: guiSettings.headerColor

        RowLayout
        {
            id: headerRow

            SvgButton {
                id: backButton
                iconColor: "white"
                Material.background: "transparent"
                svg: svgOutline.arrowBack
                onClicked: view.closed()
            }
            Text {
                id: headerTexts
                Layout.alignment: Qt.AlignVCenter
                font.bold: true
                font.pointSize: guiSettings.scaledFont(10/8)
                color: "white"
                text: qsTr("Post thread")
            }
        }
    }
    headerPositioning: ListView.OverlayHeader

    delegate: PostFeedViewDelegate {
        viewWidth: view.width
    }

    GuiSettings {
        id: guiSettings
    }

    Component.onCompleted: {
        console.debug("Entry index:", postEntryIndex);
        positionViewAtIndex(postEntryIndex, ListView.Center)
        flick(0, 0.1) // HACK: this seems to make the entry move into the visible view
    }
    Component.onDestruction: skywalker.removePostThreadModel(modelId)
}
