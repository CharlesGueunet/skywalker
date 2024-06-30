import QtQuick
import QtQuick.Controls

Label {
    property string backgroundColor: guiSettings.labelColor

    topPadding: 1
    bottomPadding: 1
    leftPadding: 2
    rightPadding: 2
    background: Rectangle { color: backgroundColor; radius: 2 }
    font.pointSize: guiSettings.labelFontSize

    Accessible.role: Accessible.StaticText
    Accessible.name: text

    GuiSettings {
        id: guiSettings
    }
}
