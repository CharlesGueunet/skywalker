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
        height: restrictionRow.visible ? guiSettings.headerHeight + restrictionRow.height : guiSettings.headerHeight
        text: qsTr("Post thread")
        onBack: view.closed()

        Rectangle {
            width: parent.width
            height: restrictionRow.height + 5
            anchors.bottom: parent.bottom
            color: guiSettings.threadStartColor
            border.width: 1
            border.color: guiSettings.headerColor
            visible: model.getReplyRestriction() !== QEnums.REPLY_RESTRICTION_NONE

            Rectangle {
                id: restrictionRow
                x: guiSettings.threadBarWidth * 5 - restrictionIcon.width
                anchors.bottom: parent.bottom
                width: parent.width - x - 10
                height: restrictionText.height + 5
                color: "transparent"

                SvgImage {
                    id: restrictionIcon
                    width: 15
                    height: 15
                    color: guiSettings.textColor
                    svg: svgOutline.replyRestrictions
                }
                Text {
                    id: restrictionText
                    anchors.left: restrictionIcon.right
                    anchors.right: parent.right
                    leftPadding: 5
                    color: guiSettings.textColor
                    font.italic: true
                    font.pointSize: guiSettings.scaledFont(7/8)
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                    textFormat: Text.RichText
                    text: restrictionRow.getRestrictionText()

                    onLinkActivated: (link) => {
                        if (link.startsWith("did:")) {
                            skywalker.getDetailedProfile(link)
                        }
                    }
                }

                function getRestrictionText() {
                    const replyRestriction = model.getReplyRestriction()

                    if (replyRestriction === QEnums.REPLY_RESTRICTION_NONE)
                        return ""

                    if (replyRestriction === QEnums.REPLY_RESTRICTION_UNKNOWN)
                        return "Replies are restricted"

                    if (replyRestriction === QEnums.REPLY_RESTRICTION_NOBODY)
                        return qsTr("Replies are disabled")

                    let restrictionList = []

                    if (replyRestriction & QEnums.REPLY_RESTRICTION_MENTIONED)
                        restrictionList.push(qsTr("mentioned users"))

                    if (replyRestriction & QEnums.REPLY_RESTRICTION_FOLLOWING) {
                        const author = model.getReplyRestrictionAuthor()
                        restrictionList.push(qsTr(`users followed by <a href="${author.did}" style="color: ${guiSettings.linkColor};">@${author.handle}</a>`))
                    }

                    if (replyRestriction & QEnums.REPLY_RESTRICTION_LIST) {
                        let lists = model.getReplyRestrictionLists()

                        for (let i = 0; i < lists.length; ++i)
                            lists[i] = `<b>${(lists[i])}</b>`

                        const names = guiSettings.toWordSequence(lists)
                        restrictionList.push(qsTr(`members of ${lists}`))
                    }

                    if (!restrictionList) {
                        console.warn("No restrictions found.")
                        return qsTr("Replies are restricted")
                    }

                    const restrictionListText = guiSettings.toWordSequence(restrictionList)
                    return qsTr(`Replies are restricted to ${restrictionListText}`)
                }
            }
        }
    }
    headerPositioning: ListView.OverlayHeader


    footer: Rectangle {
        width: parent.width
        z: guiSettings.footerZLevel

        PostButton {
            x: parent.width - width - 10
            y: -height - 10
            svg: svgOutline.reply
            overrideOnClicked: () => reply()
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

    function reply(initialText = "", imageSource = "") {
        const postUri = model.getData(postEntryIndex, AbstractPostFeedModel.PostUri)
        const postCid = model.getData(postEntryIndex, AbstractPostFeedModel.PostCid)
        const postText = model.getData(postEntryIndex, AbstractPostFeedModel.PostText)
        const postIndexedDateTime = model.getData(postEntryIndex, AbstractPostFeedModel.PostIndexedDateTime)
        const author = model.getData(postEntryIndex, AbstractPostFeedModel.Author)
        const postReplyRootUri = model.getData(postEntryIndex, AbstractPostFeedModel.PostReplyRootUri)
        const postReplyRootCid = model.getData(postEntryIndex, AbstractPostFeedModel.PostReplyRootCid)

        root.composeReply(postUri, postCid, postText, postIndexedDateTime,
                          author, postReplyRootUri, postReplyRootCid,
                          initialText, imageSource)
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
