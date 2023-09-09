import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import skywalker

ListView {
    property int margin: 8
    property int headerHeight: 44
    property int footerWeight: 44
    property int unreadPosts: 0

    property bool inTopOvershoot: false
    property bool gettingNewPosts: false
    property bool inBottomOvershoot: false
    property string avatarUrl: ""

    id: timelineView
    spacing: 0
    model: skywalker.timelineModel
    ScrollIndicator.vertical: ScrollIndicator {}

    header: Rectangle {
        width: parent.width
        height: headerHeight
        z: 10
        color: "black"

        RowLayout {
            id: headerRow
            width: parent.width
            height: headerHeight

            Text {
                id: headerTexts
                Layout.alignment: Qt.AlignVCenter
                leftPadding: 8
                font.bold: true
                font.pointSize: root.scaledFont(10/8)
                color: "white"
                text: qsTr("Following timeline")
            }
            Item {
                Layout.rightMargin: 8
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                height: parent.height - 16
                width: height

                Avatar {
                    id: avatar
                    width: parent.width
                    height: parent.height
                    avatarUrl: timelineView.avatarUrl
                }
            }
        }
    }
    headerPositioning: ListView.OverlayHeader

    footer: Rectangle {
        width: parent.width
        height: footerWeight
        z: 10
        color: "white"

        RowLayout {
            SvgButton {
                id: homeButton
                iconColor: "black"
                Material.background: "transparent"
                svg: svgOutline.home
                onClicked: moveToPost(0)

                Rectangle {
                    x: parent.width - 24
                    y: 10
                    width: Math.max(unreadCountText.width + 4, 14)
                    height: 14
                    radius: height / 2
                    color: Material.color(Material.Blue)
                    visible: timelineView.unreadPosts > 0

                    Text {
                        id: unreadCountText
                        anchors.centerIn: parent
                        font.bold: true
                        font.pointSize: root.scaledFont(6/8)
                        color: "white"
                        text: timelineView.unreadPosts
                    }
                }
            }
        }
    }
    footerPositioning: ListView.OverlayFooter

    delegate: PostFeedViewDelegate {
        viewWidth: timelineView.width
    }

    onCountChanged: {
        let firstVisibleIndex = indexAt(0, contentY)
        let lastVisibleIndex = indexAt(0, contentY + height - 1)
        console.debug("COUNT CHANGED First:", firstVisibleIndex, "Last:", lastVisibleIndex, "Count:", count)
        // Adding/removing content changes the indices.
        skywalker.timelineMovementEnded(firstVisibleIndex, lastVisibleIndex)
        updateUnreadPosts(firstVisibleIndex)
    }

    onMovementEnded: {
        let firstVisibleIndex = indexAt(0, contentY)
        let lastVisibleIndex = indexAt(0, contentY + height - 1)
        console.info("END MOVEMENT First:", firstVisibleIndex, "Last:", lastVisibleIndex, "Count:", count, "AtBegin:", atYBeginning)
        skywalker.timelineMovementEnded(firstVisibleIndex, lastVisibleIndex)
        updateUnreadPosts(firstVisibleIndex)
    }

    onVerticalOvershootChanged: {
        if (verticalOvershoot < 0)  {
            if (!inTopOvershoot && !skywalker.getTimelineInProgress) {
                gettingNewPosts = true
                skywalker.getTimeline(50)
            }

            inTopOvershoot = true
        } else {
            inTopOvershoot = false
        }

        if (verticalOvershoot > 0) {
            if (!inBottomOvershoot && !skywalker.getTimelineInProgress) {
                skywalker.getTimelineNextPage()
            }

            inBottomOvershoot = true;
        } else {
            inBottomOvershoot = false;
        }
    }

    BusyIndicator {
        id: busyTopIndicator
        y: parent.y + headerHeight
        anchors.horizontalCenter: parent.horizontalCenter
        running: gettingNewPosts
    }

    function updateUnreadPosts(firstIndex) {
        timelineView.unreadPosts = Math.max(firstIndex, 0)
    }

    function moveToPost(index) {
        positionViewAtIndex(index, ListView.Beginning)
        updateUnreadPosts(index)
    }

    Component.onCompleted: {
        skywalker.onGetTimeLineInProgressChanged.connect(() => {
                if (!skywalker.getTimelineInProgress)
                    gettingNewPosts = false
            })

        model.onRowsAboutToBeInserted.connect((parent, start, end) => {
                console.debug("ROWS TO INSERT:", start, end, "AtBegin:", atYBeginning)
            })

        model.onRowsInserted.connect((parent, start, end) => {
                console.debug("ROWS INSERTED:", start, end, "AtBegin:", atYBeginning)
                if (atYBeginning) {
                    if (count > end + 1) {
                        // Stay at the current item instead of scrolling to the new top
                        positionViewAtIndex(end, ListView.Beginning)
                    }
                    else {
                        // Avoid the flick to bounce down the timeline
                        cancelFlick()
                        positionViewAtBeginning()
                    }
                }

                let firstVisibleIndex = indexAt(0, contentY)
                updateUnreadPosts(firstVisibleIndex)
            })
    }
}
