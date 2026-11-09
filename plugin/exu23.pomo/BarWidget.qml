import QtQuick
import QtQuick.Shapes
import Quickshell
import Quickshell.Io
import qs.Commons
import qs.Ui

// Shows the Pomo app's timer in the bar. Pomo writes status.json atomically
// on every state change and deletes it on quit; with no file, or one left by
// a killed app (its pid is no longer a pomo process), the widget takes no space.
BarWidget {
  id: root
  moduleName: "exu23.pomo"

  // shortcut: the app's cycle length is fixed at four focus sessions, read from
  // its sources; the status file does not carry it. Add a field if it becomes a setting.
  readonly property int cycleLength: 4

  property var status: null
  property real nowMs: Date.now()

  readonly property bool running: status !== null && status.running === true
  property bool alive: false
  readonly property bool shown: status !== null && alive

  // The app counts down with ceil(ms / 1000), so the bar does too.
  readonly property real leftMs: !status ? 0 : (running ? Math.max(0, status.deadlineMs - nowMs) : status.leftMs)
  readonly property int leftSecs: Math.ceil(leftMs / 1000)
  readonly property real fraction: status && status.totalMs > 0 ? Math.min(1, Math.max(0, 1 - leftMs / status.totalMs)) : 0
  readonly property string timeText: Math.floor(leftSecs / 60) + ":" + ("0" + (leftSecs % 60)).slice(-2)

  readonly property color tone: !running
    ? Util.alpha(button.foreground, 0.6)
    : (status.phase === "focus" ? Color.accent : button.foreground)

  readonly property string tooltip: {
    if (!status) return ""
    var phase = status.phase === "focus"
      ? strings.t("tip.focus").arg(status.session).arg(cycleLength)
      : strings.t(status.phase === "long" ? "tip.long" : "tip.short")
    return running ? phase : phase + " · " + strings.t("tip.paused")
  }

  function parse(text) {
    var s
    try { s = JSON.parse(text) } catch (e) { return null }
    var ok = s && typeof s === "object"
      && ["focus", "short", "long"].indexOf(s.phase) >= 0
      && isFinite(s.leftMs) && isFinite(s.totalMs) && isFinite(s.deadlineMs)
    return ok ? s : null
  }

  visible: shown
  implicitWidth: shown ? button.implicitWidth : 0
  implicitHeight: button.implicitHeight

  Translations { id: strings; directory: Qt.resolvedUrl("i18n") }

  // The app replaces the file by rename, which an inotify watch on the old inode
  // would not follow, so the file is re-read on the 1 s tick instead of watched.
  FileView {
    id: file
    path: (Quickshell.env("XDG_RUNTIME_DIR") || "") + "/pomo/status.json"
    printErrors: false
    onLoaded: root.status = root.parse(text())
    onLoadFailed: root.status = null
  }

  FileView {
    id: process
    path: root.status ? "/proc/" + root.status.pid + "/comm" : ""
    printErrors: false
    onLoaded: root.alive = text().trim() === "pomo"
    onLoadFailed: root.alive = false
  }

  Timer {
    interval: 1000
    running: true
    repeat: true
    triggeredOnStart: true
    onTriggered: {
      root.nowMs = Date.now()
      file.reload()
      process.reload()
    }
  }

  WidgetButton {
    id: button
    anchors.fill: parent
    bar: root.bar
    visible: root.shown
    labelVisible: false
    hasVisualContent: root.shown
    horizontalMargin: 8.75
    fixedWidth: Math.ceil(row.width + scaledHorizontalMargin * 2)
    tooltipText: root.tooltip

    onPressed: function(b) {
      if (b === Qt.LeftButton && root.bar) root.bar.run("hyprctl dispatch focuswindow class:pomo")
    }

    Row {
      id: row
      anchors.centerIn: parent
      spacing: Style.space(6)
      scale: button.pressScale

      Item {
        id: ring
        anchors.verticalCenter: parent.verticalCenter
        width: Style.bar.iconSize
        height: width
        readonly property real stroke: 2

        Shape {
          anchors.fill: parent
          preferredRendererType: Shape.CurveRenderer
          ShapePath {
            fillColor: "transparent"
            strokeColor: Util.alpha(root.tone, 0.25)
            strokeWidth: ring.stroke
            PathAngleArc {
              centerX: ring.width / 2; centerY: ring.height / 2
              radiusX: (ring.width - ring.stroke) / 2; radiusY: radiusX
              startAngle: 0; sweepAngle: 360
            }
          }
          ShapePath {
            fillColor: "transparent"
            strokeColor: root.tone
            strokeWidth: ring.stroke
            capStyle: ShapePath.RoundCap
            Behavior on strokeColor { ColorAnimation { duration: Style.duration(160) } }
            PathAngleArc {
              centerX: ring.width / 2; centerY: ring.height / 2
              radiusX: (ring.width - ring.stroke) / 2; radiusY: radiusX
              startAngle: -90
              sweepAngle: 359.99 * root.fraction
            }
          }
        }
      }

      Text {
        id: timeLabel
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: (metrics.descent - metrics.ascent + metrics.tightBoundingRect("0").height) / 2
        textFormat: Text.PlainText
        text: root.timeText
        color: root.tone
        font.family: button.fontFamily
        font.pixelSize: button.fontSize
        font.features: { "tnum": 1 }
        renderType: Text.NativeRendering
        Behavior on color { ColorAnimation { duration: Style.duration(160) } }
        FontMetrics { id: metrics; font: timeLabel.font }
      }
    }
  }
}
