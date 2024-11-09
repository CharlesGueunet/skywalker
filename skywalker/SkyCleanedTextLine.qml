import QtQuick
import skywalker

Text {
    required property string plainText
    property bool mustClean: false
    property bool isCompleted: false

    id: theText
    color: GuiSettings.textColor
    textFormat: mustClean ? Text.RichText : Text.PlainText

    Accessible.role: Accessible.StaticText
    Accessible.name: plainText

    onPlainTextChanged: {
        if (isCompleted)
            setPlainText()
    }

    onWidthChanged: {
        if (isCompleted)
            setPlainText()
    }

    function elideText() {
        if (elide !== Text.ElideRight)
            return

        if (contentWidth <= width)
            return

        if (text.length < 2)
            return

        if (contentWidth <= 0)
            return

        const ratio = width / contentWidth
        const graphemeInfo = unicodeFonts.getGraphemeInfo(plainText)
        const newLength = Math.floor(graphemeInfo.length * ratio - 1)

        if (newLength < 1)
            return

        const elidedText = graphemeInfo.sliced(plainText, 0, newLength) + "…"
        text = unicodeFonts.toCleanedHtml(elidedText)
    }

    function setPlainText() {
        mustClean = unicodeFonts.hasCombinedEmojis(plainText)

        if (mustClean) {
            text = unicodeFonts.toCleanedHtml(plainText)
            elideText()
        }
        else {
            text = plainText
        }
    }

    UnicodeFonts {
        id: unicodeFonts
    }

    Component.onCompleted: {
        isCompleted = true
        setPlainText()
    }
}
