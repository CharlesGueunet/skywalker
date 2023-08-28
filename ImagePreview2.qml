import QtQuick
import QtQuick.Layouts
import skywalker

RoundedFrame {
    property list<imageview> images

    objectToRound: imgRow
    width: parent.width
    height: width / 2

    Row {
        id: imgRow
        anchors.fill: parent
        spacing: 4

        Image {
            id: img1
            width: parent.width / 2 - parent.spacing / 2
            height: width
            Layout.fillWidth: true
            fillMode: Image.PreserveAspectCrop
            source: images[0].thumbUrl
        }
        Image {
            id: img2
            width: parent.width / 2 - parent.spacing / 2
            height: width
            Layout.fillWidth: true
            fillMode: Image.PreserveAspectCrop
            source: images[1].thumbUrl
        }
    }
}