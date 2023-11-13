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
    flickDeceleration: guiSettings.flickDeceleration
    ScrollIndicator.vertical: ScrollIndicator {}

    header: SimpleHeader {
        text: qsTr("Post thread")
        onBack: view.closed()
    }
    headerPositioning: ListView.OverlayHeader


    footer: Rectangle {
        width: parent.width
        z: guiSettings.footerZLevel

        PostButton {
            x: parent.width - width - 10
            y: -height - 10
        }
    }
    footerPositioning: ListView.OverlayFooter

    delegate: PostFeedViewDelegate {
        viewWidth: view.width
    }

    Timer {
        id: syncTimer
        interval: 100
        onTriggered: positionViewAtIndex(postEntryIndex, ListView.Center)
    }

    GuiSettings {
        id: guiSettings
    }

    Component.onCompleted: {
        console.debug("Entry index:", postEntryIndex);

        // As not all entries have the same height, positioning at an index
        // is fickle. By moving to the end and then wait a bit before positioning
        // at the index entry, it seems to work.
        positionViewAtEnd()
        syncTimer.start()
    }
    Component.onDestruction: skywalker.removePostThreadModel(modelId)
}
