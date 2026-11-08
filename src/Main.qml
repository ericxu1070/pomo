import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 460
    height: 520
    minimumWidth: 360
    minimumHeight: 420
    visible: true
    title: backend.running ? clock(backend.remaining) + " · Pomo" : "Pomo"
    color: "#0e0e10"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false

    function clock(seconds) {
        const m = Math.floor(seconds / 60), s = seconds % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    function phaseName(phase) {
        return phase === "focus" ? qsTr("Focus") : phase === "short" ? qsTr("Short break") : qsTr("Long break")
    }

    function quit() {
        if (backend.running) quitDialog.open()
        else Qt.quit()
    }

    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Esc"; context: Qt.ApplicationShortcut; enabled: win.helpVisible; onActivated: win.helpVisible = false }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: win.quit() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.toggle() }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: backend.reset() }
    Shortcut { sequence: "S"; context: Qt.ApplicationShortcut; onActivated: backend.skip() }

    onClosing: (close) => {
        if (backend.running) {
            close.accepted = false
            quitDialog.open()
        }
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - 48
            spacing: 18

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: win.phaseName(backend.phase)
                font.pixelSize: 22
                color: backend.phase === "focus" ? theme.accent : "#c8c8cc"
            }

            Item {
                id: dial
                Layout.alignment: Qt.AlignHCenter
                readonly property int size: Math.min(win.width - 80, win.height - 220)
                Layout.preferredWidth: size
                Layout.preferredHeight: size

                Canvas {
                    id: ring
                    anchors.fill: parent
                    readonly property real progress: backend.total > 0 ? 1 - backend.remaining / backend.total : 0
                    readonly property string accent: theme.accent
                    onProgressChanged: requestPaint()
                    onAccentChanged: requestPaint()
                    onWidthChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.reset()
                        const r = width / 2 - 8
                        ctx.lineWidth = 8
                        ctx.strokeStyle = "#26262b"
                        ctx.beginPath()
                        ctx.arc(width / 2, height / 2, r, 0, 2 * Math.PI)
                        ctx.stroke()
                        if (progress > 0) {
                            ctx.strokeStyle = accent
                            ctx.lineCap = "round"
                            ctx.beginPath()
                            ctx.arc(width / 2, height / 2, r, -Math.PI / 2, -Math.PI / 2 + progress * 2 * Math.PI)
                            ctx.stroke()
                        }
                    }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: win.clock(backend.remaining)
                        font.pixelSize: Math.max(24, Math.round(dial.size / 4.5))
                        font.weight: Font.Light
                        font.features: { "tnum": 1 }
                        color: "white"
                    }
                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: backend.running ? qsTr("Space to pause")
                              : backend.remaining < backend.total ? qsTr("Paused") : qsTr("Space to start")
                        color: "#8a8a90"
                        font.pixelSize: 14
                    }
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("%n sessions today", "", backend.completedToday)
                color: "#c8c8cc"
                font.pixelSize: 15
            }

            Label {
                Layout.fillWidth: true
                visible: backend.error !== ""
                text: backend.error
                color: "#ff6b6b"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Label {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 14
            text: qsTr("? for shortcuts")
            color: "#5a5a60"
            font.pixelSize: 12
        }

        // Every shortcut, toggled with ?.
        Rectangle {
            anchors.fill: parent
            visible: win.helpVisible
            color: "#e00e0e10"

            MouseArea { anchors.fill: parent; onClicked: win.helpVisible = false }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 32, 360)
                height: helpColumn.implicitHeight + 40
                radius: theme.radius
                color: "#1a1a1e"
                border.color: "#2e2e34"

                ColumnLayout {
                    id: helpColumn
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10

                    Label { text: qsTr("Keyboard shortcuts"); font.pixelSize: 17; color: "white" }

                    GridLayout {
                        columns: 2
                        columnSpacing: 18
                        rowSpacing: 8
                        Repeater {
                            model: [
                                ["Space", qsTr("Start or pause")],
                                ["R", qsTr("Reset this timer")],
                                ["S", qsTr("Skip to the next phase")],
                                ["?", qsTr("Show or hide shortcuts")],
                                ["Q", qsTr("Quit")]
                            ]
                            delegate: RowLayout {
                                Layout.columnSpan: 2
                                spacing: 18
                                Rectangle {
                                    Layout.preferredWidth: 64
                                    Layout.preferredHeight: 26
                                    radius: Math.min(4, theme.radius)
                                    color: "#2a2a30"
                                    Label { anchors.centerIn: parent; text: modelData[0]; color: theme.accent }
                                }
                                Label { text: modelData[1]; color: "#c8c8cc"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: quitDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Quit while the timer runs?")
        Material.roundedScale: theme.radius
        background: Rectangle { color: "#1a1a1e"; radius: theme.radius; border.color: "#2e2e34" }
        Overlay.modal: Rectangle { color: "#c00e0e10" }
        header: Label {
            text: quitDialog.title
            padding: 24
            bottomPadding: 0
            font.pixelSize: 18
            color: "white"
            wrapMode: Text.WordWrap
        }
        Label { text: qsTr("The current session won't be counted."); wrapMode: Text.WordWrap; width: 280 }
        footer: DialogButtonBox {
            background: null
            Button { text: qsTr("Keep going"); flat: true; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: qsTr("Quit"); flat: true; DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
        }
        onAccepted: Qt.quit()
    }
}
