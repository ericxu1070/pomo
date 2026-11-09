import QtQuick

// A key and what it does. Nothing here is clickable: it shows the keyboard, and
// lights for a moment when its key is pressed.
Row {
    id: hint
    property var keys: []
    property string label
    property bool lit: false
    spacing: 6
    opacity: enabled ? 1 : 0.4

    Repeater {
        model: hint.keys
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(24, cap.implicitWidth + 14)
            height: 24
            radius: pal.radius.sm
            color: hint.lit ? pal.a(pal.tint, 0.35) : pal.fillPressed
            Behavior on color { ColorAnimation { duration: hint.lit ? 0 : 200 } }
            Txt { id: cap; anchors.centerIn: parent; role: "footnote"; font.family: "JetBrains Mono"; text: modelData }
        }
    }
    Txt {
        anchors.verticalCenter: parent.verticalCenter
        role: "callout"
        text: hint.label
        color: pal.labelSecondary
        visible: text !== ""
    }
}
