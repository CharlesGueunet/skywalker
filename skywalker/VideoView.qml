import QtQuick
import QtQuick.Controls
import QtMultimedia
import skywalker

Column {
    required property var videoView // videoView
    required property int contentVisibility // QEnums::ContentVisibility
    required property string contentWarning
    property string controlColor: guiSettings.textColor
    property string disabledColor: guiSettings.disabledColor
    property string backgroundColor: "transparent"
    property bool highlight: false
    property string borderColor: highlight ? guiSettings.borderHighLightColor : guiSettings.borderColor
    property int maxHeight: 0
    property bool isFullViewMode: false
    readonly property bool isPlaying: videoPlayer.playing || videoPlayer.restarting
    property var userSettings: root.getSkywalker().getUserSettings()
    property string videoSource
    property string transcodedSource // Could be the same as videoSource if transcoding failed or not needed
    property bool autoLoad: userSettings.videoAutoPlay || userSettings.videoAutoLoad

    // Cache
    property list<string> tmpVideos: []

    id: videoStack
    spacing: isFullViewMode ? -playControls.height : 10

    Rectangle {
        width: parent.width
        height: videoColumn.height
        color: "transparent"
        visible: videoPlayer.videoFound || videoPlayer.error == MediaPlayer.NoError

        FilteredImageWarning {
            id: filter
            x: 10
            y: 10
            width: parent.width - 20
            contentVisibiliy: videoStack.contentVisibility
            contentWarning: videoStack.contentWarning
            imageUrl: videoView.thumbUrl
            isVideo: true
        }

        Column {
            id: videoColumn
            width: parent.width
            topPadding: 1
            spacing: 3

            Rectangle {
                width: parent.width
                height: filter.height > 0 ? filter.height + 20 : 0
                color: "transparent"
            }

            Rectangle {
                id: imgPreview
                width: parent.width
                height: defaultThumbImg.visible ? defaultThumbImg.height : thumbImg.height
                color: "transparent"

                ThumbImageView {
                    property double aspectRatio: implicitHeight > 0 ? implicitWidth / implicitHeight : 0
                    property double maxWidth: maxHeight * aspectRatio

                    id: thumbImg
                    x: (parent.width - width) / 2
                    width: parent.width - 2
                    imageView: filter.imageVisible() ? videoView.imageView : filter.nullImage
                    fillMode: Image.PreserveAspectFit
                    enableAlt: !isFullViewMode

                    onWidthChanged: Qt.callLater(setSize)
                    onHeightChanged: Qt.callLater(setSize)

                    function setSize() {
                        if (maxWidth > 0 && width > maxWidth)
                            height = maxHeight
                        else if (aspectRatio > 0)
                            height = width / aspectRatio
                    }

                    Component.onCompleted: setSize()
                }
                Rectangle {
                    property double maxWidth: maxHeight * videoStack.getAspectRatio()

                    id: defaultThumbImg
                    x: (parent.width - width) / 2
                    width: (maxWidth > 0 && parent.width - 2 > maxWidth) ? maxWidth : parent.width - 2
                    height: width / videoStack.getAspectRatio()
                    color: guiSettings.avatarDefaultColor
                    visible: videoView.imageView.isNull() || thumbImg.status != Image.Ready && filter.imageVisible()

                    onHeightChanged: {
                        if (maxHeight && height > maxHeight)
                            Qt.callLater(setMaxHeight)
                    }

                    function setMaxHeight() {
                        const ratio = width / height
                        width = Math.floor(maxHeight * ratio)
                    }

                    SkySvg {
                        x: (parent.width - width) / 2
                        y: (parent.height - height) / 2 + height
                        width: Math.min(parent.width, 150)
                        height: width
                        color: "white"
                        svg: SvgFilled.film
                    }
                }

                SkyLabel {
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 5
                    backgroundColor: "black"
                    backgroundOpacity: 0.6
                    color: "white"
                    text: guiSettings.videoDurationToString(videoPlayer.getDuration())
                    visible: !isFullViewMode
                }
            }
        }

        SvgButton {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 50
            height: width
            opacity: 0.5
            accessibleName: qsTr("play video")
            svg: SvgFilled.play
            visible: filter.imageVisible() && !videoPlayer.playing && !videoPlayer.restarting
            enabled: videoPlayer.hasVideo || !autoLoad

            onClicked: {
                if (videoSource)
                    videoPlayer.start()
                else if (!autoLoad)
                    m3u8Reader.loadStream()
            }

            BusyIndicator {
                anchors.fill: parent
                running: parent.visible && !parent.enabled && (videoPlayer.error == MediaPlayer.NoError || videoPlayer.videoFound)
            }
        }

        Item {
            id: player
            width: parent.width
            height: parent.height

            MediaPlayer {
                property bool videoFound: false
                property bool restarting: false
                property bool positionKicked: false
                property bool mustKickPosition: false // hack for playing live stream
                property int m3u8DurationMs: 0

                id: videoPlayer
                source: transcodedSource
                loops: MediaPlayer.Infinite
                videoOutput: videoOutput
                audioOutput: audioOutput

                onHasVideoChanged: {
                    if (hasVideo)
                        videoFound = true
                }

                onPlayingChanged: {
                    if (videoPlayer.playing) {
                        restartTimer.set(false)
                    }
                }

                onPositionChanged: {
                    if (!mustKickPosition || positionKicked)
                        return

                    console.debug("POSITION:", position)

                    // HORRIBLE HACK
                    // Qt fails to play the first part properly. Resetting the position
                    // like this makes it somewhat better
                    if (position > 100) {
                        positionKicked = true
                        position = position - 50
                        console.debug("POSITION KICKED")
                    }
                }

                onPlaybackStateChanged: {
                    if (mustKickPosition && playbackState === MediaPlayer.StoppedState)
                    {
                        source = ""
                        source = videoSource
                    }
                }

                onErrorOccurred: (error, errorString) => { console.debug("Video error:", source, error, errorString) }

                function getDuration() {
                    return duration === 0 ? m3u8DurationMs : duration
                }

                function playPause() {
                    if (playbackState == MediaPlayer.PausedState)
                        play()
                    else
                        pause()
                }

                function start() {
                    restartTimer.set(true)
                    play()
                }

                function stopPlaying() {
                    stop()
                    restartTimer.set(false)
                }

                function isLoading() {
                    return mediaStatus === MediaPlayer.LoadingMedia
                }
            }
            VideoOutput {
                id: videoOutput
                anchors.fill: parent
            }
            AudioOutput {
                id: audioOutput
                muted: !userSettings.videoSound || userSettings.videoAutoPlay

                function toggleSound() {
                    if (userSettings.videoAutoPlay)
                        muted = !muted
                    else
                        userSettings.videoSound = !userSettings.videoSound
                }
            }

            // Adding the condition on transcoding to the first busy indicator
            // did not work as there is short moment when loading has stopped
            // and transcoding not started yet. Whem transcoding starts the busy
            // inidcator will not restart anymore.
            BusyIndicator {
                anchors.centerIn: parent
                running: (videoPlayer.playing && videoPlayer.isLoading()) ||
                         videoPlayer.restarting ||
                         (m3u8Reader.loading && !autoLoad)
            }
            BusyIndicator {
                anchors.centerIn: parent
                running: videoUtils.transcoding && !autoLoad
            }

            Timer {
                id: restartTimer
                interval: 5000
                onTriggered: {
                    videoPlayer.stop()
                    videoPlayer.restarting = false
                    console.warn("Failed to start video")
                }

                function set(on) {
                    videoPlayer.restarting = on

                    if (on)
                        start()
                    else
                        stop()
                }
            }
        }

        MouseArea {
            width: parent.width
            height: parent.height
            z: -1
            onClicked: {
                if (isFullViewMode)
                    playControls.show = !playControls.show
                else
                    root.viewFullVideo(videoView, videoSource, transcodedSource)
            }
        }

        RoundCornerMask {
            width: parent.width
            height: parent.height
            maskColor: videoStack.backgroundColor == "transparent" ? guiSettings.backgroundColor : videoStack.backgroundColor
            visible: !isFullViewMode
        }
    }

    Rectangle {
        property bool show: true

        id: playControls
        x: (parent.width - width) / 2
        width: defaultThumbImg.visible ? defaultThumbImg.width : Math.min(thumbImg.width, thumbImg.maxWidth ? thumbImg.maxWidth : thumbImg.width)
        height: playPauseButton.height
        color: "transparent"
        visible: show && (videoPlayer.playbackState == MediaPlayer.PlayingState || videoPlayer.playbackState == MediaPlayer.PausedState || videoPlayer.restarting)

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.3
            visible: isFullViewMode
        }

        SvgTransparentButton {
            id: playPauseButton
            x: isFullViewMode ? 10 : 0
            width: 32
            height: width
            svg: videoPlayer.playing ? SvgFilled.pause : SvgFilled.play
            accessibleName: videoPlayer.playing ? qsTr("pause video") : qsTr("continue playing video")
            color: controlColor

            onClicked: videoPlayer.playPause()
        }

        SvgTransparentButton {
            id: stopButton
            anchors.left: playPauseButton.right
            anchors.leftMargin: 10
            width: 32
            height: width
            svg: SvgFilled.stop
            accessibleName: qsTr("stop video")
            color: controlColor

            onClicked: videoPlayer.stopPlaying()
        }

        ProgressBar {
            id: playProgress
            anchors.left: stopButton.right
            anchors.leftMargin: 15
            anchors.right: remainingTimeText.left
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            from: 0
            to: videoPlayer.duration
            value: videoPlayer.position

            background: Rectangle {
                implicitWidth: parent.width
                implicitHeight: 4
                color: disabledColor
            }

            contentItem: Item {
                implicitWidth: parent.width
                implicitHeight: 4

                Rectangle {
                    width: playProgress.visualPosition * parent.width
                    height: parent.height
                    color: controlColor
                }
            }

            Rectangle {
                x: playProgress.visualPosition * playProgress.availableWidth
                y: playProgress.topPadding + playProgress.availableHeight / 2 - height / 2

                width: 10
                height: width
                radius: width / 2
                color: controlColor
            }
        }

        MouseArea {
            x: playProgress.x
            width: playProgress.width
            height: playControls.height
            onClicked: (event) => {
                videoPlayer.position = (videoPlayer.duration / width) * event.x
            }
            onPositionChanged: (event) => {
                videoPlayer.position = (videoPlayer.duration / width) * event.x
            }
        }

        AccessibleText {
            id: remainingTimeText
            anchors.right: soundButton.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            font.pointSize: guiSettings.scaledFont(6/8)
            color: controlColor
            text: guiSettings.videoDurationToString(videoPlayer.duration - videoPlayer.position)
        }

        SvgTransparentButton {
            id: soundButton
            anchors.right: parent.right
            anchors.rightMargin: isFullViewMode ? 10 : 0
            width: visible ? 32 : 0
            height: width
            svg: audioOutput.muted ? SvgOutline.soundOff : SvgOutline.soundOn
            accessibleName: audioOutput.muted ? qsTr("turn sound on") : qsTr("turn sound off")
            color: controlColor
            visible: videoPlayer.hasAudio

            onClicked: audioOutput.toggleSound()
        }
    }

    Rectangle {
        width: parent.width
        height: errorText.height
        radius: 10
        border.width: 1
        border.color: videoStack.borderColor
        color: "transparent"
        visible: !videoPlayer.videoFound && videoPlayer.error != MediaPlayer.NoError

        AccessibleText {
            id: errorText
            padding: 10
            text: qsTr(`⚠️ Video error: ${videoPlayer.errorString}`)
        }
    }

    M3U8Reader {
        id: m3u8Reader

        onGetVideoStreamOk: (durationMs) => {
            videoPlayer.m3u8DurationMs = durationMs

            if (autoLoad)
                loadStream()
        }

        onGetVideoStreamError: {
            videoSource = videoView.playlistUrl
            transcodedSource = videoSource
            videoPlayer.mustKickPosition = true

            if (userSettings.videoAutoPlay)
                videoPlayer.start()
        }

        onLoadStreamOk: (videoStream) => {
            videoSource = videoStream

            // TS streams do not loop well in the media player, also seeking works mediocre.
            // Therefore we transcode to MP4
            if (videoStream.endsWith(".ts")) {
                console.debug("Transcode to MP4:", videoStream)
                videoUtils.transcodeVideo(videoStream.slice(7), -1, -1, -1, false)
            }
            else if (!autoLoad || userSettings.videoAutoPlay) {
                transcodedSource = videoSource
                videoPlayer.start()
            }
        }

        onLoadStreamError: {
            console.warn("Could not load stream")
        }
    }

    VideoUtils {
        id: videoUtils

        onTranscodingOk: (inputFileName, outputFileName) => {
            console.debug("Set MP4 source:", outputFileName)
            transcodedSource = "file://" + outputFileName
            videoStack.tmpVideos.push(transcodedSource)

            if (!autoLoad || userSettings.videoAutoPlay)
                videoPlayer.start()
        }

        onTranscodingFailed: (inputFileName, errorMsg) => {
            console.debug("Could not transcode to MP4:", inputFileName, "error:", errorMsg)
            transcodedSource = videoSource

            if (!autoLoad || userSettings.videoAutoPlay)
                videoPlayer.start()
        }
    }

    function pause() {
        if (videoPlayer.playing)
        {
            if (!userSettings.videoAutoPlay)
                videoPlayer.pause()
            else
                audioOutput.muted = true
        }
    }

    function getAspectRatio() {
        if (videoView && videoView.width > 0 && videoView.height > 0)
            return videoView.width / videoView.height
        else
            return 16/9
    }

    function getDurationMs() {
        return videoPlayer.getDuration()
    }

    function setVideoSource() {
        console.debug("Set video source for:", videoView.playlistUrl)

        if (videoView.playlistUrl.endsWith(".m3u8")) {
            m3u8Reader.getVideoStream(videoView.playlistUrl)
        }
        else {
            videoSource = videoView.playlistUrl
            transcodedSource = videoSource

            if (userSettings.videoAutoPlay)
                videoPlayer.start()
        }
    }

    Component.onDestruction: {
        tmpVideos.forEach((value, index, array) => { videoUtils.dropVideo(value); })
    }

    Component.onCompleted: {
        if (!videoSource)
            setVideoSource()
    }
}
