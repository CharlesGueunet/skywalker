import QtQuick
import QtQuick.Layouts
import skywalker

Column {
    property basicprofile author
    property string postText
    property date postDateTime

    id: quoteColumn
    padding: 10

    RowLayout {
        width: parent.width - 20

        Avatar {
            id: avatar
            width: 24
            Layout.alignment: Qt.AlignTop
            avatarUrl: author.avatarUrl
        }

        PostHeader {
            Layout.fillWidth: true
            authorName: author.name
            authorHandle: author.handle
            postThreadType: QEnums.THREAD_NONE
            postIndexedSecondsAgo: (new Date() - postDateTime) / 1000
        }
    }

    PostBody {
        width: parent.width - 20
        postText: quoteColumn.postText
        postImages: []
        postDateTime: postDateTime
        maxTextLines: 5
    }
}