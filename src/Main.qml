import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Shapes

ApplicationWindow {
    id: win
    width: 480
    height: 600
    minimumWidth: 440
    minimumHeight: 480
    visible: true
    title: backend.running ? clock(backend.remaining) + " · Pomo" : "Pomo"
    color: pal.bg
    font.family: pal.family

    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.accent: theme.accent

    Pal { id: pal }

    readonly property bool started: backend.remaining < backend.total
    readonly property bool blocked: quitSheet.visible
    property string lit: ""

    function clock(seconds) {
        const m = Math.floor(seconds / 60), s = seconds % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    function phaseName(phase) {
        return phase === "focus" ? qsTr("Focus") : phase === "short" ? qsTr("Short break") : qsTr("Long break")
    }

    function nextLine() {
        const minutes = Math.round(backend.nextTotal / 60)
        return backend.nextPhase === "focus" ? qsTr("Focus next, %1 min").arg(minutes)
             : backend.nextPhase === "short" ? qsTr("Short break next, %1 min").arg(minutes)
             : qsTr("Long break next, %1 min").arg(minutes)
    }

    function press(key, action) {
        lit = key
        litTimer.restart()
        action()
    }

    function quit() {
        if (backend.running) quitSheet.visible = true
        else Qt.quit()
    }

    Timer { id: litTimer; interval: 150; onTriggered: win.lit = "" }

    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; enabled: !win.blocked; onActivated: win.press("?", () => helpSheet.visible = !helpSheet.visible) }
    Shortcut { sequence: "Esc"; context: Qt.ApplicationShortcut; enabled: helpSheet.visible || quitSheet.visible; onActivated: helpSheet.visible = quitSheet.visible = false }
    Shortcut { sequences: ["Return", "Enter"]; context: Qt.ApplicationShortcut; enabled: quitSheet.visible; onActivated: Qt.quit() }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; enabled: !win.blocked; onActivated: win.quit() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; enabled: !win.blocked; onActivated: win.press("space", backend.toggle) }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; enabled: !win.blocked; onActivated: win.press("r", backend.reset) }
    Shortcut { sequence: "S"; context: Qt.ApplicationShortcut; enabled: !win.blocked; onActivated: win.press("s", backend.skip) }
    // = is + without Shift.
    Shortcut { sequences: ["+", "="]; context: Qt.ApplicationShortcut; enabled: !win.blocked; onActivated: win.press("length", () => backend.adjust(5)) }
    Shortcut { sequence: "-"; context: Qt.ApplicationShortcut; enabled: !win.blocked; onActivated: win.press("length", () => backend.adjust(-5)) }

    onClosing: (close) => {
        if (backend.running) {
            close.accepted = false
            quitSheet.visible = true
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
            spacing: 0

            Txt {
                Layout.alignment: Qt.AlignHCenter
                role: "title2"
                text: win.phaseName(backend.phase)
            }

            Item {
                id: dial
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                readonly property int size: Math.max(160, Math.min(win.width - 120, win.height - 320))
                readonly property real progress: backend.total > 0 ? 1 - backend.remaining / backend.total : 0
                Layout.preferredWidth: size
                Layout.preferredHeight: size

                Shape {
                    anchors.fill: parent
                    preferredRendererType: Shape.CurveRenderer

                    ShapePath {
                        strokeColor: pal.fillPressed
                        strokeWidth: 6
                        fillColor: "transparent"
                        PathAngleArc {
                            centerX: dial.size / 2; centerY: dial.size / 2
                            radiusX: dial.size / 2 - 3; radiusY: radiusX
                            sweepAngle: 360
                        }
                    }
                    ShapePath {
                        strokeColor: pal.tint
                        strokeWidth: 6
                        capStyle: ShapePath.RoundCap
                        fillColor: "transparent"
                        PathAngleArc {
                            centerX: dial.size / 2; centerY: dial.size / 2
                            radiusX: dial.size / 2 - 3; radiusY: radiusX
                            startAngle: -90
                            sweepAngle: 360 * dial.progress
                        }
                    }
                }

                Txt {
                    id: readout
                    anchors.centerIn: parent
                    text: win.clock(backend.remaining)
                    font.pixelSize: Math.round(dial.size / 4.2)
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.02 * dial.size / 4.2
                    font.features: { "tnum": 1 }
                }
                Txt {
                    anchors.top: readout.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !backend.running && win.started
                    role: "callout"
                    text: qsTr("Paused")
                    color: pal.labelSecondary
                }
            }

            // One pill per focus session in the set of four; the current one fills as it runs.
            Row {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                spacing: 6
                Repeater {
                    model: 4
                    Rectangle {
                        width: 32; height: 4; radius: 2
                        color: pal.fillPressed
                        Rectangle {
                            height: 4; radius: 2
                            color: pal.tint
                            width: parent.width * (index < backend.cycleDone ? 1
                                   : index === backend.cycleDone && backend.phase === "focus" ? dial.progress : 0)
                        }
                    }
                }
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 12
                Txt { role: "headline"; text: qsTr("Session %1 of %2").arg(backend.session).arg(4) }
                Txt { text: " · " + win.nextLine(); color: pal.labelSecondary }
            }

            Txt {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                role: "callout"
                text: qsTr("%n sessions today", "", backend.completedToday)
                color: pal.labelSecondary
            }

            Item {
                Layout.fillWidth: true
                Layout.topMargin: 32
                implicitHeight: hints.implicitHeight
              Row {
                id: hints
                anchors.centerIn: parent
                spacing: 16
                // Longer languages shrink the row instead of running off the window.
                scale: Math.min(1, parent.width / implicitWidth)
                Keycap {
                    keys: ["Space"]
                    lit: win.lit === "space"
                    label: backend.running ? qsTr("Pause") : win.started ? qsTr("Resume") : qsTr("Start")
                }
                Keycap { keys: ["R"]; lit: win.lit === "r"; label: qsTr("Reset"); enabled: win.started }
                Keycap { keys: ["S"]; lit: win.lit === "s"; label: qsTr("Skip") }
                Keycap { keys: ["−", "+"]; lit: win.lit === "length"; label: qsTr("Length"); enabled: !backend.running }
                Keycap { keys: ["?"]; lit: win.lit === "?" }
              }
            }

            Txt {
                Layout.fillWidth: true
                Layout.topMargin: 16
                visible: backend.error !== ""
                role: "callout"
                text: backend.error
                color: pal.danger
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Sheet {
            id: helpSheet
            onDismissed: visible = false

            Txt { role: "title3"; text: qsTr("Keyboard shortcuts") }

            Column {
                width: parent.width
                spacing: 8
                Repeater {
                    model: [
                        [["Space"], qsTr("Start or pause")],
                        [["R"], qsTr("Reset this timer")],
                        [["S"], qsTr("Skip to the next phase")],
                        [["−", "+"], qsTr("Change the length by 5 min")],
                        [["?"], qsTr("Show or hide shortcuts")],
                        [["Q"], qsTr("Quit")]
                    ]
                    Item {
                        width: parent.width
                        height: 24
                        Txt { anchors.verticalCenter: parent.verticalCenter; text: modelData[1] }
                        Keycap { anchors.right: parent.right; keys: modelData[0] }
                    }
                }
            }
        }

        Sheet {
            id: quitSheet
            onDismissed: visible = false

            Txt { role: "title3"; text: qsTr("Quit while the timer runs?"); width: parent.width; wrapMode: Text.WordWrap }
            Txt { text: qsTr("The current session won't be counted."); color: pal.labelSecondary; width: parent.width; wrapMode: Text.WordWrap }
            Row {
                anchors.right: parent.right
                spacing: 16
                Keycap { keys: ["Esc"]; label: qsTr("Keep going") }
                Keycap { keys: ["Enter"]; label: qsTr("Quit") }
            }
        }
    }
}
