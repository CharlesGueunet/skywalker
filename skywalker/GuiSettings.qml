﻿import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import skywalker

// I would like this as singleton. However the properties do not update
// when Material.theme changes in a singleton. (Bug in Qt??)
Item {
    property Skywalker skywalker: root.getSkywalker()
    readonly property var userSettings: skywalker ? skywalker.getUserSettings() : null

    // Geometry
    readonly property int footerHeight: 50
    readonly property int footerZLevel: 10
    readonly property int headerHeight: 50
    readonly property int headerZLevel: 10
    readonly property int labelHeight: labelFontHeight + 2
    readonly property int labelRowPadding: 5
    readonly property int statsHeight: appFontHeight + 4
    readonly property int threadColumnWidth: 60
    readonly property int threadLineWidth: 2

    // Colors
    readonly property string accentColor: userSettings ? userSettings.accentColor : (Material.theme === Material.Light ? "blue" : "#58a6ff")
    readonly property string avatarDefaultColor: accentColor
    readonly property string backgroundColor: userSettings ? userSettings.backgroundColor : Material.background
    readonly property string badgeBorderColor: backgroundColor
    readonly property string badgeColor: accentColor
    readonly property string badgeTextColor: "white"
    readonly property string bannerDefaultColor: accentColor
    readonly property string borderColor: Material.theme === Material.Light ? Qt.darker(backgroundColor, 1.1) : Qt.lighter(backgroundColor, 1.6)
    readonly property string borderHighLightColor: Material.theme === Material.Light ? Qt.darker(borderColor, 1.1) : Qt.lighter(borderColor, 1.6)
    readonly property string buttonColor: accentColor
    readonly property string buttonFlatColor: Material.theme === Material.Light ? Qt.lighter(buttonColor, 1.8) : Qt.darker(buttonColor, 1.8)
    readonly property string buttonNeutralColor: Material.theme === Material.Light ? Qt.darker(backgroundColor, 1.07) : Qt.lighter(backgroundColor, 1.6) //Material.background : "darkslategrey"
    readonly property string buttonTextColor: "white"
    readonly property string contentLabelColor: Material.theme === Material.Light ? "#f3f3f3" : "#1d3030"
    readonly property string contentUserLabelColor: Material.theme === Material.Light ? "lightblue" : "steelblue"
    readonly property string disabledColor: Material.theme === Material.Light ? "lightgrey" : "darkslategrey"
    readonly property string errorColor: Material.theme === Material.Light ? "darkred" : "palevioletred"
    readonly property string favoriteColor: "gold"
    readonly property string footerColor: backgroundColor
    readonly property string handleColor: Material.color(Material.Grey)
    readonly property string headerColor: "black"
    readonly property string headerHighLightColor: Material.theme === Material.Light ? "lightblue" : "darkslategrey"
    readonly property string headerTextColor: "white"
    readonly property string labelColor: Material.theme === Material.Light ? "lightblue" : "steelblue"
    readonly property string likeColor: "palevioletred"
    readonly property string linkColor: userSettings ? userSettings.linkColor : (Material.theme === Material.Light ? "blue" : "#58a6ff")
    readonly property string messageTimeColor: Material.color(Material.Grey)
    readonly property string messageNewBackgroundColor: Material.theme === Material.Light ? "#f3f3f3" : "#1d3030"
    readonly property string messageNewTextColor: Material.theme === Material.Light ? "black" : "white"
    readonly property string messageUserBackgroundColor: Material.theme === Material.Light ? "#58a6ff" : "#0053b3"
    readonly property string messageUserTextColor: "white"
    readonly property string messageOtherBackgroundColor: Material.theme === Material.Light ? "#f3f3f3" : "#1d3030"
    readonly property string messageOtherTextColor: Material.theme === Material.Light ? "black" : "white"
    readonly property string moderatorIconColor: "lightgrey"
    readonly property string placeholderTextColor: Material.color(Material.Grey)
    readonly property string postHighLightColor: Material.theme === Material.Light ? Qt.darker(backgroundColor, 1.1) : Qt.lighter(backgroundColor, 1.6)
    readonly property string selectionColor: accentColor
    readonly property string separatorColor: Material.theme === Material.Light ? Qt.darker(backgroundColor, 1.08) : Qt.lighter(backgroundColor, 1.6)
    readonly property string separatorHighLightColor: Material.theme === Material.Light ? Qt.darker(separatorColor, 1.1) : Qt.lighter(separatorColor, 1.6)
    readonly property string skywalkerLogoColor: "#0387c7"
    readonly property string statsColor: Material.color(Material.Grey)
    readonly property string textColor: Material.foreground
    readonly property string textLengthExceededColor: "palevioletred"

    // Opacity
    readonly property double focusHighlightOpacity: 0.2

    // Font size
    readonly property double appFontHeight: fontMetrics.height
    readonly property double labelFontHeight: appFontHeight * 6/8
    readonly property double labelFontSize: scaledFont(6/8)

    // Misc
    readonly property real flickDeceleration: 800
    readonly property real maxFlickVelocity: 12000
    readonly property bool flickPixelAligned: false
    readonly property bool isAndroid: Qt.platform.os === "android"

    // Identity
    readonly property string skywalkerHandle: "@skywalker.thereforeiam.eu"

    Accessible.ignored: true

    FontMetrics {
        id: fontMetrics
        font: Application.font
    }

    function scaledFont(scaleFactor) {
        return Application.font.pointSize * scaleFactor;
    }

    function isToday(date) {
        const today = new Date()
        return date.getDate() === today.getDate() &&
            date.getMonth() === today.getMonth() &&
            date.getFullYear() === today.getFullYear()
    }

    function isYesterday(date) {
        const nextDay = new Date(new Date().setDate(date.getDate() + 1))
        return isToday(nextDay)
    }

    function durationToString(duration) {
        if (duration < 59.5)
            return Math.round(duration) + qsTr("s", "seconds")

        duration = duration / 60
        if (duration < 59.5)
            return Math.round(duration) + qsTr("m", "minutes")

        duration = duration / 60
        if (duration < 23.5)
            return Math.round(duration) + qsTr("h", "hours")

        duration = duration / 24
        if (duration < 30.4368499)
            return Math.round(duration) + qsTr("d", "days")

        duration = duration / 30.4368499
        if (duration < 35.5)
            return Math.round(duration) + qsTr("mo", "months")

        duration = duration / 12
        return Math.round(duration) + qsTr("yr", "years")
    }

    function videoDurationToString(durationMs) {
        const minutes = Math.floor(durationMs / 60000)
        const seconds = String(Math.ceil((durationMs - minutes * 60000) / 1000)).padStart(2, '0')
        return `${minutes}:${seconds}`
    }

    function askDiscardSaveQuestion(parent, question, onDiscardCb, onSaveCb) {
        let component = Qt.createComponent("Message.qml")
        let message = component.createObject(parent, { standardButtons: Dialog.No | Dialog.Discard | Dialog.Save })
        message.onDiscarded.connect(() => { message.destroy(); onDiscardCb() })
        message.onAccepted.connect(() => { message.destroy(); onSaveCb() })
        message.onRejected.connect(() => message.destroy())
        message.show(question)
    }

    function askYesNoQuestion(parent, question, onYesCb, onNoCb = () => {}) {
        let component = Qt.createComponent("Message.qml")
        let message = component.createObject(parent, { standardButtons: Dialog.Yes | Dialog.No })
        message.onAccepted.connect(() => { message.destroy(); onYesCb() })
        message.onRejected.connect(() => { message.destroy(); onNoCb() })
        message.show(question)
    }

    function notice(parent, msg, emoji = "", onOkCb = () => {}) {
        let component = Qt.createComponent("Message.qml")
        let message = component.createObject(parent, { emoji: emoji, standardButtons: Dialog.Ok })
        message.onAccepted.connect(() => { message.destroy(); onOkCb() })
        message.onRejected.connect(() => message.destroy())
        message.show(msg)
    }

    function askConvertGif(parent, gifSource, onVideoCb, onImageCb) {
        let component = Qt.createComponent("ConvertGifDialog.qml")
        let dialog = component.createObject(parent, { gifSource: gifSource })
        dialog.onAccepted.connect(() => { dialog.destroy(); onVideoCb() })
        dialog.onRejected.connect(() => { dialog.destroy(); onImageCb() })
        dialog.open()
    }

    function showProgress(parent, msg, onCancelCb) {
        let component = Qt.createComponent("ProgressDialog.qml")
        let dialog = component.createObject(parent)
        dialog.onRejected.connect(() => { dialog.destroy(); onCancelCb() })
        dialog.show(msg)
        return dialog
    }

    function toWordSequence(stringList) {
        if (!stringList)
            return ""

        let wordSequence = stringList[0]

        for (let i = 1; i < stringList.length - 1; ++i)
            wordSequence += ", " + stringList[i]

        if (stringList.length > 1) {
            wordSequence += qsTr(" and ")
            wordSequence += stringList[stringList.length - 1]
        }

        return wordSequence
    }

    function listTypeName(purpose) {
        switch (purpose) {
        case QEnums.LIST_PURPOSE_MOD:
            return qsTr("Moderation List")
        case QEnums.LIST_PURPOSE_CURATE:
            return qsTr("User List")
        case QEnums.LIST_PURPOSE_REFERENCE:
            return qsTr("Reference List")
        default:
            return qsTr("List")
        }
    }

    function getFocusHashtagEntryText(entry) {
        let text = ""

        for (let i = 0; i < entry.hashtags.length; ++i) {
            const tag = entry.hashtags[i]

            if (i > 0)
                text += ' '

            text += `<a href="${tag}" style="color: ${guiSettings.linkColor}; text-decoration: none">#${tag}</a>`
        }

        console.debug("TEXT:", text)
        return text
    }

    function contentVisible(author)
    {
        if (author.viewer.blockedBy)
            return false

        let visibility = skywalker.getContentVisibility(author.labels)
        return visibility === QEnums.CONTENT_VISIBILITY_SHOW
    }

    function filterContentLabelsToShow(contentLabels) {
        let contentFilter = skywalker.getContentFilter()
        let labels = []

        for (let i = 0; i < contentLabels.length; ++i) {
            const label = contentLabels[i]

            if (!label.isSystemLabel() && contentFilter.mustShowBadge(label))
                labels.push(label)
        }

        return labels
    }

    function threadStartColor(color) {
        return color
    }

    function threadMidColor(color) {
        return Material.theme === Material.Light ?  Qt.lighter(threadStartColor(color), 1.3) : Qt.darker(threadStartColor(color), 1.3)
    }

    function threadEndColor(color) {
        return Material.background
    }

    function threadEntryColor(color) {
        return Material.theme === Material.Light ? Qt.darker(threadStartColor(color), 1.2) : Qt.lighter(threadStartColor(color), 1.2)
    }

    function forbiddenBackgroundColors() {
        return [textColor,
                badgeColor,
                buttonColor,
                contentLabelColor,
                contentUserLabelColor,
                handleColor,
                likeColor,
                linkColor,
                messageTimeColor,
                messageNewBackgroundColor,
                messageUserBackgroundColor,
                messageOtherBackgroundColor,
                statsColor]
    }

    function forbiddenAccentColors() {
        return [backgroundColor,
                buttonTextColor,
                headerColor]
    }

    function forbiddenLinkColors() {
        return [backgroundColor,
                messageNewBackgroundColor,
                messageUserBackgroundColor,
                messageOtherBackgroundColor,
                postHighLightColor]
    }
}
